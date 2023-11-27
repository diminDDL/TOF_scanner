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
#define CALIB_STAND_VAL 0.19
#define YAW_STOP 1498


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

void core1()
{
    while (true)
    {
        sleep_ms(500);
        //printf("-30.0\n");
        pitch_setpoint = pitch_to_us(-40.0);
        sleep_ms(500);
        yaw_setpoint = (1510);
        sleep_ms(1000);
        yaw_setpoint = (1485);
        sleep_ms(1000);
        yaw_setpoint = (1500);
        sleep_ms(1000);
        //printf("0.0\n");
        pitch_setpoint = pitch_to_us(0.0);
        sleep_ms(500);
        //printf("90.0\n");
        pitch_setpoint = pitch_to_us(90.0);
        sleep_ms(500);
        //printf("180.0\n");
        pitch_setpoint = pitch_to_us(180.0);
        sleep_ms(2000);
    }
}

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
    printf("Distance measurement: 0.0\n");
}

void setScanResolution(float resolution) {
    // Implement scan resolution setting
    printf("Scan resolution set to: %f\n", resolution);
}

void setScanAngleYaw(float angle) {
    // Implement scan angle setting on yaw axis
    printf("Scan angle set to: %f\n", angle);
}

void setScanAnglePitch(float angle) {
    // Implement scan angle setting on pitch axis
    printf("Scan angle set to: %f\n", angle);
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
        setScanResolution(atof(arg));
    } else if (strcmp(command, "scyaw") == 0 && arg != NULL) {
        setScanAngleYaw(atof(arg));
    } else if (strcmp(command, "scpitc") == 0 && arg != NULL) {
        setScanAnglePitch(atof(arg));
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


bool calibrate_yaw(PWM &pwm_yaw, PWM &pwm_pitch, float threshold){
    // position the pitch to it's minimum
    pwm_pitch.set_duty_cycle_us(pitch_to_us(-1.0 * PITCH_OFFSET));
    // tell the user to move yaw close to the calibration ledge
    printf("Move yaw to the calibration ledge\n");
    // wait for the user to move the yaw to the calibration ledge, user will send 'done' when ready
    while(true){
        readSerial(false);
        if(strcmp(readBuff, "done\r\n") == 0){
            break;
        }else{
            printf(readBuff);
        }
        tight_loop_contents();
    }
    uint step_size = 20;
    uint step_delay = 25;
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
        float distance = tof_measure_distance();
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
            float distance = tof_measure_distance();
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
    // TODO: finish the time measurement to get a deg to us conversion
    return true;
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

    while (true)
    {

        // read in the required angles and points per degree in the following format:
        // deg_yaw(float);deg_pitch(float);points_deg(int)\n

        
        float distance = tof_measure_distance();
        //printf("%f\n", distance);
        // if (distance < CALIB_STAND_VAL){
        //     printf("FOUND STAND\n");
        // }
        sleep_ms(50);
        readSerial();
        if(calibration){
           bool res = calibrate_yaw(pwm_yaw, pwm_pitch, CALIB_STAND_VAL);
           printf("calibration exited with - %d", res);
           calibration = false;
        }
        // pwm_pitch.set_duty_cycle_us(pitch_setpoint);
        // pwm_yaw.set_duty_cycle_us(yaw_setpoint);
        
    }
}