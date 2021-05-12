
#include "bme280_spi.hpp"


// Initialize BME280 sensor
BME280::BME280(  uint spi_no    = 0, 
                 uint rx_pin    = PICO_DEFAULT_SPI_RX_PIN, 
                 uint tx_pin    = PICO_DEFAULT_SPI_TX_PIN, 
                 uint sck_pin   = PICO_DEFAULT_SPI_SCK_PIN, 
                 uint cs_pin    = PICO_DEFAULT_SPI_CSN_PIN, 
                 uint freq      = 500 * 1000,
                 MODE mode      = MODE::MODE_NORMAL) {

    this->spi_no            = spi_no;
    this->rx_pin            = rx_pin;
    this->tx_pin            = tx_pin;
    this->sck_pin           = sck_pin;
    this->cs_pin            = cs_pin;
    this->freq              = freq;
    measurement_reg.mode    = mode;
       
    switch (spi_no) {
        case 0: spi_hw = spi0;
                break;
        case 1: spi_hw = spi1;
                break;
        default: return;
    }
    // initialize SPI access
    spi_init(spi_hw, freq);

     // set all SPI GPIOs   
    gpio_set_function(rx_pin, GPIO_FUNC_SPI);
    gpio_set_function(sck_pin, GPIO_FUNC_SPI);     
    gpio_set_function(tx_pin, GPIO_FUNC_SPI);
    // Make the SPI pins available to picotool
    bi_decl(bi_3pins_with_func(rx_pin, tx_pin, sck_pin, GPIO_FUNC_SPI));
    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_init(cs_pin);
    gpio_set_dir(cs_pin, GPIO_OUT);
    gpio_put(cs_pin, 1);
    // Make the CS pin available to picotool
    bi_decl(bi_1pin_with_name(cs_pin, "SPI CS"));

    // See if SPI is working - interrograte the device for its I2C ID number, should be 0x60
    read_registers(0xD0, &chip_id, 1);
  
    // read compensation params once
    read_compensation_parameters();
    
    measurement_reg.osrs_p = 0b011; // x4 Oversampling
    measurement_reg.osrs_t = 0b011; // x4 Oversampling
    write_register(0xF4, MODE::MODE_SLEEP); //SLEEP_MODE ensures configuration is saved
 
    // save configuration
    write_register(0xF2, 0x1); // Humidity oversampling register - going for x1
    write_register(0xF4, measurement_reg.get());// Set rest of oversampling modes and run mode to normal
};

BME280::Measurement_t BME280::measure() {
    int32_t pressure, humidity, temperature;
    if (measurement_reg.mode = MODE::MODE_FORCED) {
        write_register(0xf4, measurement_reg.get());
        uint8_t buffer;
        do {
            read_registers(0xf3, &buffer, 1);
            sleep_ms(1);
        } while (buffer & 0x08); // loop until measurement completed
    }
    // read raw sensor data from BME280
    bme280_read_raw(&humidity,
                    &pressure,
                    &temperature);
    // compensate raw sensor values
    pressure = compensate_pressure(pressure);
    humidity = compensate_humidity(humidity);
    temperature = compensate_temp(temperature);
    measurement.pressure = pressure / 100.0;
    measurement.humidity = humidity / 1024.0;
    measurement.temperature = temperature / 100.0;
    // apply formula to retrieve altitude from air pressure
    // pressure at sea level required as a base
    float pressure0 = 1013.25;
    float tmp = pow(measurement.pressure / pressure0, 1.0 / 5.255);
    measurement.altitude = (measurement.temperature + 273.15) * (1 - tmp) / (tmp * 0.0065);
    
    return measurement;
}

uint8_t BME280::get_chipID() {
    return chip_id;
}

// for the compensate_functions read the Bosch information on the BME280
int32_t BME280::compensate_temp(int32_t adc_T) {
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t) dig_T1 << 1))) * ((int32_t) dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t) dig_T1)) * ((adc_T >> 4) - ((int32_t) dig_T1))) >> 12) * ((int32_t) dig_T3))
            >> 14;

    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

uint32_t BME280::compensate_pressure(int32_t adc_P) {
    int32_t var1, var2;
    uint32_t p;
    var1 = (((int32_t) t_fine) >> 1) - (int32_t) 64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t) dig_P6);
    var2 = var2 + ((var1 * ((int32_t) dig_P5)) << 1);
    var2 = (var2 >> 2) + (((int32_t) dig_P4) << 16);
    var1 = (((dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int32_t) dig_P2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((int32_t) dig_P1)) >> 15);
    if (var1 == 0)
        return 0;

    p = (((uint32_t) (((int32_t) 1048576) - adc_P) - (var2 >> 12))) * 3125;
    if (p < 0x80000000)
        p = (p << 1) / ((uint32_t) var1);
    else
        p = (p / (uint32_t) var1) * 2;

    var1 = (((int32_t) dig_P9) * ((int32_t) (((p >> 3) * (p >> 3)) >> 13))) >> 12;
    var2 = (((int32_t) (p >> 2)) * ((int32_t) dig_P8)) >> 13;
    p = (uint32_t) ((int32_t) p + ((var1 + var2 + dig_P7) >> 4));

    return p;
}

uint32_t BME280::compensate_humidity(int32_t adc_H) {
    int32_t v_x1_u32r;
    v_x1_u32r = (t_fine - ((int32_t) 76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t) dig_H4) << 20) - (((int32_t) dig_H5) * v_x1_u32r)) +
                   ((int32_t) 16384)) >> 15) * (((((((v_x1_u32r * ((int32_t) dig_H6)) >> 10) * (((v_x1_u32r *
                                                                                                  ((int32_t) dig_H3))
            >> 11) + ((int32_t) 32768))) >> 10) + ((int32_t) 2097152)) *
                                                 ((int32_t) dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t) dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

    return (uint32_t) (v_x1_u32r >> 12);
}

// select BME280 sensor on SPI bus
inline void BME280::cs_select() {
    asm volatile("nop \n nop \n nop");
    gpio_put(cs_pin, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

// deselect BME280 sensor on SPI bus
inline void BME280::cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(cs_pin, 1);
    asm volatile("nop \n nop \n nop");
}

void BME280::write_register(uint8_t reg, uint8_t data) {
    uint8_t buf[2];
    buf[0] = reg & 0x7f;  // remove read bit as this is a write
    buf[1] = data;
    cs_select();
    spi_write_blocking(spi_hw, buf, 2);
    cs_deselect();
    sleep_ms(10);
}

void BME280::read_registers(uint8_t reg, uint8_t *buf, uint16_t len) {
    // For this particular device, we send the device the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing
    // so we don't need to keep sending the register we want, just the first.
    reg |= READ_BIT;
    cs_select();
    spi_write_blocking(spi_hw, &reg, 1);
    sleep_ms(10);
    spi_read_blocking(spi_hw, 0, buf, len);
    cs_deselect();
    sleep_ms(10);
}


/* This function reads the manufacturing assigned compensation parameters from the device */
void BME280::read_compensation_parameters() {
   

    read_registers(0x88, buffer, 26);

    dig_T1 = buffer[0] | (buffer[1] << 8);
    dig_T2 = buffer[2] | (buffer[3] << 8);
    dig_T3 = buffer[4] | (buffer[5] << 8);

    dig_P1 = buffer[6] | (buffer[7] << 8);
    dig_P2 = buffer[8] | (buffer[9] << 8);
    dig_P3 = buffer[10] | (buffer[11] << 8);
    dig_P4 = buffer[12] | (buffer[13] << 8);
    dig_P5 = buffer[14] | (buffer[15] << 8);
    dig_P6 = buffer[16] | (buffer[17] << 8);
    dig_P7 = buffer[18] | (buffer[19] << 8);
    dig_P8 = buffer[20] | (buffer[21] << 8);
    dig_P9 = buffer[22] | (buffer[23] << 8);

    dig_H1 = buffer[25];

    read_registers(0xE1, buffer, 8);

    dig_H2 = buffer[0] | (buffer[1] << 8);
    dig_H3 = (int8_t) buffer[2];
    dig_H4 = buffer[3] << 4 | (buffer[4] & 0xf);
    dig_H5 = (buffer[5] >> 4) | (buffer[6] << 4);
    dig_H6 = (int8_t) buffer[7];
}

// this functions reads the raw data values from the sensor
void BME280::bme280_read_raw(int32_t *humidity, int32_t *pressure, int32_t *temperature) {
    uint8_t readBuffer[8];
    read_registers(0xF7, readBuffer, 8);
    *pressure = ((uint32_t) readBuffer[0] << 12) | ((uint32_t) readBuffer[1] << 4) | (readBuffer[2] >> 4);
    *temperature = ((uint32_t) readBuffer[3] << 12) | ((uint32_t) readBuffer[4] << 4) | (readBuffer[5] >> 4);
    *humidity = (uint32_t) readBuffer[6] << 8 | readBuffer[7];
}
