#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "pwm.pio.h"
#include "tusb.h"
#include "lib/pico_tof/picotof.hpp"
#include "lib/pico_tof/eeprom.hpp"
#include "lib/pico_tof/isl29501.hpp"
#include "lib/pwm.hpp"

#define PITCH_OFFSET 40.0
#define CALIB_STAND_VAL 0.16
#define YAW_STOP 1495
#define BASE_YAW_STEP 20
#define BASE_YAW_DELAY 50
#define PITCH_STOP 1500
#define AVG_SAMPLES 5

const uint sda_pin = 12;
const uint scl_pin = 13;

const uint ss_pin = 14;
const uint irq_pin = 15;

const uint yaw_servo_pin = 0;
const uint pitch_servo_pin = 1;

bool reserved_addr(uint8_t addr)
{
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void test_read(i2c_inst_t *i2c, uint8_t device_addr)
{
    uint8_t data[16];
    for (uint16_t addr = 0; addr < 512; addr += 16)
    {
        EEPROM_PageRead(i2c, device_addr, addr, data, 16);
        // Print or check the read data
        printf("Address: %03X Data: ", addr);
        for (int i = 0; i < 16; ++i)
        {
            printf("%02X ", data[i]);
        }
        printf("\n");
    }
}

uint8_t ISL29501_ReadDeviceID(i2c_inst_t *i2c, uint8_t device_addr)
{
    uint8_t deviceID;
    if (ISL29501_Read(i2c, device_addr, 0x00, &deviceID, 1))
    {
        printf("Device ID: 0x%02x\n", deviceID);
    }
    else
    {
        printf("Failed to read Device ID\n");
    }
    return deviceID;
}

float deg_yaw = 0.0;
float deg_pitch = 0.0;
uint points_per_deg = 1;
bool scan_in_progress = false;

uint pitch_to_us(float pitch)
{
    // 0.0 -> 1000
    // 180.0 -> 2000
    return (uint)((pitch + PITCH_OFFSET) * 5.5555) + 1000;
}

uint pitch_setpoint = 1500;
uint yaw_setpoint = 1500;

bool calibration = false;

void initiateCalibration() {
    // Implement calibration logic
    printf("Calibration initiated\n");
    calibration = true;
}

void initiateEstop() {
    // Implement emergency stop logic
    printf("Emergency stop initiated\n");
}

void getCurrentDistanceMeasurement() {
    // Implement distance measurement retrieval
    printf("Distance measurement: %f\n", tof_measure_distance());
}

void setScanResolution(int resolution) {
    if(resolution < 1 || resolution > 10){
        printf("Invalid resolution: %d, range is 1 - 10\n", resolution);
        return;
    }
    points_per_deg = resolution;
    // Implement scan resolution setting
    printf("Scan resolution set to: %d\n", resolution);
}

void setScanAngleYaw(float angle) {
    if(angle < 10.0 || angle > 360.0){
        printf("Invalid angle: %f, range is 10.0 - 360.0\n", angle);
        return;
    }
    // Implement scan angle setting on yaw axis
    printf("Scan angle set to: %f\n", angle);
    deg_yaw = angle;
}

void setScanAnglePitch(float angle) {
    if(angle < 1.0 || angle > 180.0){
        printf("Invalid angle: %f, range is 1.0 - 180.0\n", angle);
        return;
    }
    // Implement scan angle setting on pitch axis
    printf("Scan angle set to: %f\n", angle);
    deg_pitch = angle;
}

void startScan() {
    scan_in_progress = true;
    printf("Scan started\n");
}

void processCommand(char* fullCommand){
    char *command = strtok(fullCommand, " ");
    char *arg = strtok(NULL, " ");

    if (strcmp(command, "calib") == 0) {
        initiateCalibration();
    } else if (strcmp(command, "estop") == 0) {
        initiateEstop();
    } else if (strcmp(command, "gdis") == 0) {
        getCurrentDistanceMeasurement();
    } else if (strcmp(command, "scres") == 0 && arg != NULL) {
        setScanResolution(atoi(arg));
    } else if (strcmp(command, "scyaw") == 0 && arg != NULL) {
        setScanAngleYaw(atof(arg));
    } else if (strcmp(command, "scpitc") == 0 && arg != NULL) {
        setScanAnglePitch(atof(arg));
    } else if (strcmp(command, "scstart") == 0) {
        startScan();
    } else {
        printf("Invalid command or missing argument: %s\n", fullCommand);
    }
}

bool newData = false;
char readBuff[128] = {0};

void updateValues(void){
    char *pch;
    pch = strtok(readBuff, "\r\n");
    while (pch != NULL) {
        processCommand(pch);
        pch = strtok(NULL, "\r\n");
    }
    memset(readBuff, 0, sizeof(readBuff));
}

void readSerial(bool update = true){
    uint32_t i = 0;
    uint32_t size = sizeof(readBuff);
    if (tud_cdc_available()){
        memset(readBuff, 0, size);
    }
    while (tud_cdc_available()) {
        readBuff[i] = (char)tud_cdc_read_char();
        i++;
        newData = true;
        if (i >= size) {
            break;
        }
    }
    if (newData) {
        if(update)
            updateValues();
        newData = false;
    }
}

float average_tof_distance(uint samples){
    float sum = 0.0;
    for(uint i = 0; i < samples; i++){
        sum += tof_measure_distance();
    }
    return sum / (float)samples;
}

// because of the stupid continuous "servo" it's impossible to have repeatable motion in both directions
// so we have to calibrate the yaw servo to find out how many steps it takes to get to the ledge
// and in which direction to do so, so we need to use these for the actual measurements
uint unit_steps_per_rev = 0;
bool unit_dir = true;
float deg_per_unit_step = 0.0;
bool calibrate_yaw(PWM &pwm_yaw, PWM &pwm_pitch, float threshold){
    // position the pitch to it's minimum
    pwm_pitch.set_duty_cycle_us(pitch_to_us(-1.0 * PITCH_OFFSET + 10));
    // tell the user to move yaw close to the calibration ledge
    printf("Move yaw to the calibration ledge\n");
    // wait for the user to move the yaw to the calibration ledge, user will send 'done' when ready
    while(true){
        readSerial(false);
        if(strcmp(readBuff, "done\r\n") == 0){
            break;
        }
        tight_loop_contents();
    }
    bool dir = true;
    uint step_size = BASE_YAW_STEP;
    uint step_delay = BASE_YAW_DELAY;
    uint step_timeout = 15;
    // now we move yaw to the left until we hit the ledge or timeout if we timeout, we start turning right
    // until we hit the ledge or timeout, if we timeout again, we return false
    // if we hit the ledge, we continue the calibration process
    while(true){
        // move the yaw to the left
        pwm_yaw.set_duty_cycle_us(YAW_STOP + step_size);
        // wait for the servo to move
        sleep_ms(step_delay);
        // stop the servo
        pwm_yaw.set_duty_cycle_us(YAW_STOP);
        // wait for the servo to stop
        sleep_ms(step_delay);
        // read the distance
        float distance = average_tof_distance(AVG_SAMPLES);
        printf("moving left: %f\n", distance);
        // if we are close enough to the ledge, we are done
        if(distance < threshold){
            printf("FOUND STAND\n");
            break;
        }
        // if we have timed out, we start turning right
        if(step_timeout == 0){
            break;
        }
        // decrement the timeout
        step_timeout--;
    }
    // if we timed out, we start turning right
    if(step_timeout == 0){
        dir = false;
        step_timeout = 30;
        // now we move yaw to the right until we hit the ledge or timeout if we timeout, we return false
        // if we hit the ledge, we continue the calibration process
        while(true){
            // move the yaw to the right
            pwm_yaw.set_duty_cycle_us(YAW_STOP - step_size);
            // wait for the servo to move
            sleep_ms(step_delay);
            // stop the servo
            pwm_yaw.set_duty_cycle_us(YAW_STOP);
            // wait for the servo to stop
            sleep_ms(step_delay);
            // read the distance
            float distance = average_tof_distance(AVG_SAMPLES);
            printf("moving right: %f\n", distance);
            // if we are close enough to the ledge, we are done
            if(distance < threshold){
                printf("FOUND STAND\n");
                break;
            }
            // if we have timed out, we return false
            if(step_timeout == 0){
                return false;
            }
            // decrement the timeout
            step_timeout--;
        }
    }
    // now we move into th the opposite direction until we hit the ledge again, recording how many steps it took
    // to get there
    uint steps = 0;
    step_timeout = 500;
    // first we move until we stop seeing the ledge
    while(true){
        // move the yaw to the left
        if(dir){
            pwm_yaw.set_duty_cycle_us(YAW_STOP - step_size);
        }else{
            pwm_yaw.set_duty_cycle_us(YAW_STOP + step_size);
        }
        // wait for the servo to move
        sleep_ms(step_delay);
        // stop the servo
        pwm_yaw.set_duty_cycle_us(YAW_STOP);
        // wait for the servo to stop
        sleep_ms(step_delay);
        // read the distance
        float distance = average_tof_distance(AVG_SAMPLES);
        printf("moving %s: %f\n", dir ? "left" : "right", distance);
        // if we are far enough from the ledge, we are done
        if(distance > threshold){
            printf("LOST STAND\n");
            break;
        }
        // if we have timed out, we return false
        if(step_timeout == 0){
            return false;
        }
        // decrement the timeout
        step_timeout--;
        // increment the steps
        steps++;
    }
    // then we move until we see the ledge again
    while(true){
        // move the yaw to the right
        if(dir){
            pwm_yaw.set_duty_cycle_us(YAW_STOP - step_size);
        }else{
            pwm_yaw.set_duty_cycle_us(YAW_STOP + step_size);
        }
        // wait for the servo to move
        sleep_ms(step_delay);
        // stop the servo
        pwm_yaw.set_duty_cycle_us(YAW_STOP);
        // wait for the servo to stop
        sleep_ms(step_delay);
        // read the distance
        float distance = average_tof_distance(AVG_SAMPLES);
        printf("moving %s: %f\n", dir ? "left" : "right", distance);
        // if we are close enough to the ledge, we are done
        if(distance < threshold){
            printf("FOUND STAND\n");
            break;
        }
        // if we have timed out, we return false
        if(step_timeout == 0){
            return false;
        }
        // decrement the timeout
        step_timeout--;
        // increment the steps
        steps++;
    }
    printf("steps: %d\n", steps);
    unit_steps_per_rev = steps;
    unit_dir = dir;
    deg_per_unit_step = 360.0 / (float)steps;
    return true;
}

void core1()
{
    while (true)
    {
        readSerial();
        sleep_ms(50);
    }
}

int main()
{

    stdio_init_all();

    PWM pwm_yaw(yaw_servo_pin, pwm_gpio_to_slice_num(yaw_servo_pin));
    pwm_yaw.init();
    pwm_yaw.set_freq(50);
    // pwm_yaw.set_duty_cycle(50);
    pwm_yaw.set_duty_cycle_us(YAW_STOP);
    // 1530 = stop, or just stop the pwm
    pwm_yaw.set_enabled(true);

    PWM pwm_pitch(pitch_servo_pin, pwm_gpio_to_slice_num(pitch_servo_pin));
    pwm_pitch.init();
    pwm_pitch.set_freq(50);
    // pwm_pitch.set_duty_cycle(50);
    pwm_pitch.set_duty_cycle_us(2000);
    // 1530 = stop, or just stop the pwm
    pwm_pitch.set_enabled(true);

    multicore_launch_core1(core1);

    static const uint led_pin = 25;
    static const float pio_freq = 2000;

    // PIO pio = pio0;

    // uint sm = pio_claim_unused_sm(pio, true);

    // uint offset = pio_add_program(pio, &blink_program);

    // float div = (float)clock_get_hz(clk_sys) / pio_freq;

    // blink_program_init(pio, sm, offset, led_pin, div);

    // pio_sm_set_enabled(pio, sm, true);

    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    gpio_put(led_pin, 1);

    // i2c_init(i2c0, 100 * 1000);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);

    sleep_ms(2000);
    printf("configuring eeprom\n");
    // try to intialize the eeporm
    if(!EEPROM_begin(i2c0, eeprom_addr)){
        printf("Boy you fucked up I2C EEPROM no worky :(\n");
        while(1){
            tight_loop_contents();
        }
    }

    test_read(i2c0, eeprom_addr);

    printf("configuring tof\n");
    // test teh ISL
    ISL29501_ReadDeviceID(i2c0, isl29501_addr);

    // try to initialize it and read it
    tof_init(i2c0, irq_pin, ss_pin);

    sleep_ms(10);

    bool calibrated = false; 

    float current_yaw = 0.0;
    float current_pitch = 0.0;

    while (true)
    {

        // read in the required angles and points per degree in the following format:
        // deg_yaw(float);deg_pitch(float);points_deg(int)\n

        
        // float distance = tof_measure_distance();
        //printf("%f\n", distance);
        // if (distance < CALIB_STAND_VAL){
        //     printf("FOUND STAND\n");
        // }
        sleep_ms(50);
        //readSerial();
        if(calibration){
           calibrated = calibrate_yaw(pwm_yaw, pwm_pitch, CALIB_STAND_VAL);
           calibration = false;
        }
        if(scan_in_progress && calibrated){
            // position the pitch to it's minimum
            pwm_pitch.set_duty_cycle_us(pitch_to_us(-1.0 * PITCH_OFFSET + 10));
            for(uint i = 0; i < unit_steps_per_rev; i++){
                for(uint j = 0; j < (deg_pitch / points_per_deg); j++){
                    // set the pitch
                    pwm_pitch.set_duty_cycle_us(pitch_to_us(-1.0 * PITCH_OFFSET + (float)j * points_per_deg));
                    // wait for the servo to move
                    sleep_ms(50);
                    // read the distance
                    float distance = average_tof_distance(AVG_SAMPLES);
                    // calculate the current pitch in degrees
                    current_pitch = -1.0 * PITCH_OFFSET + (float)j * points_per_deg;
                    // print the values
                    printf("%f;%f;%f\n", current_pitch, current_yaw, distance);
                }
                if(unit_dir){
                    pwm_yaw.set_duty_cycle_us(YAW_STOP - BASE_YAW_STEP);
                }else{
                    pwm_yaw.set_duty_cycle_us(YAW_STOP + BASE_YAW_STEP);
                }
                // wait for the servo to move
                sleep_ms(BASE_YAW_DELAY);
                // stop the servo
                pwm_yaw.set_duty_cycle_us(YAW_STOP);
                // wait for the servo to stop
                sleep_ms(BASE_YAW_DELAY);

                if(current_yaw > deg_yaw){
                    break;
                }

                // calculate the current yaw in degrees
                if(unit_dir){
                    current_yaw = deg_per_unit_step * (float)i;
                }else{
                    current_yaw -= deg_per_unit_step * (float)i;
                }
            }
            // scan complete
            printf("Scan complete\n");

            scan_in_progress = false;
        }else if(scan_in_progress){
            printf("Calibration not complete\n");
            scan_in_progress = false;
        }
        // pwm_pitch.set_duty_cycle_us(pitch_setpoint);
        // pwm_yaw.set_duty_cycle_us(yaw_setpoint);
        
    }
}