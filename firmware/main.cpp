#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "pwm.pio.h"
#include "lib/pico_tof/picotof.hpp"
#include "lib/pico_tof/eeprom.hpp"
#include "lib/pico_tof/isl29501.hpp"

const uint sda_pin = 12;
const uint scl_pin = 13;

const uint ss_pin = 14;
const uint irq_pin = 15;


bool reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void test_read(i2c_inst_t *i2c, uint8_t device_addr) {
    uint8_t data[16];
    for(uint16_t addr = 0; addr < 512; addr += 16) {
        EEPROM_PageRead(i2c, device_addr, addr, data, 16);
        // Print or check the read data
        printf("Address: %03X Data: ", addr);
        for(int i = 0; i < 16; ++i) {
            printf("%02X ", data[i]);
        }
        printf("\n");
    }
}

uint8_t ISL29501_ReadDeviceID(i2c_inst_t *i2c, uint8_t device_addr) {
    uint8_t deviceID;
    if(ISL29501_Read(i2c, device_addr, 0x00, &deviceID, 1)) {
        printf("Device ID: 0x%02x\n", deviceID);
    } else {
        printf("Failed to read Device ID\n");
    }
    return deviceID;
}

int main() {

    stdio_init_all();

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
        printf("%f\n", tof_measure_distance());
        //sleep_ms(10);
    }
    

}