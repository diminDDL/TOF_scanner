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

const uint8_t tof_addr = 0x57;

struct TOFConfig {
    uint8_t integ_per[2];
    uint8_t samp_per[2];
    uint8_t samp_cont[2];
    uint8_t magic[2];
    uint8_t agc_cont[2];
    uint8_t irq_cont[2];
    uint8_t driv_rang[2];
    uint8_t emmit_dac[2];
} __attribute__((packed));

TOFConfig tof_config = {
    {0x10, 0x04},
    {0x11, 0x6E},
    {0x13, 0x0A},
    {0x18, 0x22},
    {0x19, 0x22},
    {0x60, 0x01},
    {0x90, 0x0F},
    {0x91, 0xFF}
};

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

    // write the default values to the sensor
    // uint8_t buf[2];
    // for (int i = 0; i < sizeof(tof_config); i += 2) {
    //     buf[0] = ((uint8_t*)&tof_config)[i];
    //     buf[1] = ((uint8_t*)&tof_config)[i + 1];
    //     i2c_write_blocking(i2c0, tof_addr, buf, 2, false);
    //     // printf("wrote %02x %02x\n", buf[0], buf[1]);
    // }

    while (true)
    {
        uint8_t ErroCode = 0;
        char SerialNo[12];
	    int N = 100, sum = 0;
	    int distance_val, distance_val_avg;


        tight_loop_contents();
    }
    

}