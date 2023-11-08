#include "i2c.hpp"
#include "driver/i2c.h"

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"

int i2c_master_port = 0;
i2c_config_t conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = 0,         // select SDA GPIO specific to your project
    .scl_io_num = 26,         // select SCL GPIO specific to your project
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master = {
        .clk_speed = 100000,  // https://github.com/espressif/esp-idf/blob/master/examples/peripherals/i2c/i2c_simple/main/i2c_simple_main.c#L28C37-L28C43
    },
    .clk_flags = 0,
};

uint8_t *i2c_buf;

void i2c_init(void) {
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0));
}


// Start borrowed code

int i2cdevRead(uint8_t devAddress, uint16_t len, uint8_t *data)
{
    return i2cdevReadReg8(devAddress, I2CDEV_NO_MEM_ADDR, len, data);
}

int i2cdevReadByte(uint8_t devAddress, uint8_t memAddress,
                    uint8_t *data)
{
    return i2cdevReadReg8(devAddress, memAddress, 1, data);
}

int i2cdevReadBit(uint8_t devAddress, uint8_t memAddress,
                   uint8_t bitNum, uint8_t *data)
{
    uint8_t byte;
    int status;

    status = i2cdevReadReg8(devAddress, memAddress, 1, &byte);
    *data = byte & (1 << bitNum);

    return status;
}

int i2cdevReadBits( uint8_t devAddress, uint8_t memAddress,
                    uint8_t bitStart, uint8_t length, uint8_t *data)
{
    int status;
    uint8_t byte;

    if ((status = i2cdevReadByte(devAddress, memAddress, &byte)) == true) {
        uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
        byte &= mask;
        byte >>= (bitStart - length + 1);
        *data = byte;
    }

    return status;
}

int i2cdevReadReg8(uint8_t devAddress, uint8_t memAddress,
                    uint16_t len, uint8_t *data)
{
    // if (xSemaphoreTake(dev->isBusFreeMutex, (TickType_t)5) == pdFALSE) {
    //     return false;
    // }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (memAddress != I2CDEV_NO_MEM_ADDR) {
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (devAddress << 1) | I2C_MASTER_WRITE, I2C_MASTER_ACK_EN);
        i2c_master_write_byte(cmd, memAddress, I2C_MASTER_ACK_EN);
    }
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (devAddress << 1) | I2C_MASTER_READ, I2C_MASTER_ACK_EN);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmd, (TickType_t)5);
    i2c_cmd_link_delete(cmd);

    // xSemaphoreGive(dev->isBusFreeMutex);

#if defined CONFIG_I2CBUS_LOG_READWRITES

    if (!err) {
        char str[length * 5 + 1];

        for (size_t i = 0; i < length; i++) {
            sprintf(str + i * 5, "0x%s%X ", (data[i] < 0x10 ? "0" : ""), data[i]);
        }

        I2CBUS_LOG_RW("[port:%d, slave:0x%X] Read_ %d bytes from register 0x%X, data: %s", port, devAddr, length, regAddr, str);
    }

#endif
#if defined CONFIG_I2CBUS_LOG_ERRORS
#if defined CONFIG_I2CBUS_LOG_READWRITES
    else {
#else

    if (err) {
#endif
        I2CBUS_LOGE("[port:%d, slave:0x%X] Failed to read %d bytes from register 0x%X, error: 0x%X",
                    port, devAddr, length, regAddr, err);
    }

#endif

    return !err;
}

int i2cdevReadReg16(uint8_t devAddress, uint16_t memAddress,
                     uint16_t len, uint8_t *data)
{
    // if (xSemaphoreTake(dev->isBusFreeMutex, (TickType_t)5) == pdFALSE) {
    //     return false;
    // }

    uint8_t memAddress8[2];
    memAddress8[0] = (uint8_t)((memAddress >> 8) & 0x00FF);
    memAddress8[1] = (uint8_t)(memAddress & 0x00FF);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (memAddress != I2C_NO_INTERNAL_ADDRESS) {
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (devAddress << 1) | I2C_MASTER_WRITE, I2C_MASTER_ACK_EN);
        i2c_master_write(cmd, memAddress8, 2, I2C_MASTER_ACK_EN);
    }
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (devAddress << 1) | I2C_MASTER_READ, I2C_MASTER_ACK_EN);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmd, (TickType_t)5);
    i2c_cmd_link_delete(cmd);

    // xSemaphoreGive(dev->isBusFreeMutex);

#if defined CONFIG_I2CBUS_LOG_READWRITES

    if (!err) {
        char str[length * 5 + 1];

        for (size_t i = 0; i < length; i++) {
            sprintf(str + i * 5, "0x%s%X ", (data[i] < 0x10 ? "0" : ""), data[i]);
        }

        I2CBUS_LOG_RW("[port:%d, slave:0x%X] Read_ %d bytes from register 0x%X, data: %s", port, devAddr, length, regAddr, str);
    }

#endif
#if defined CONFIG_I2CBUS_LOG_ERRORS
#if defined CONFIG_I2CBUS_LOG_READWRITES
    else {
#else

    if (err) {
#endif
        I2CBUS_LOGE("[port:%d, slave:0x%X] Failed to read %d bytes from register 0x%X, error: 0x%X",
                    port, devAddr, length, regAddr, err);
    }

#endif

    return !err;
}

int i2cdevWriteByte(uint8_t devAddress, uint8_t memAddress,
                     uint8_t data)
{
    return i2cdevWriteReg8(devAddress, memAddress, 1, &data);
}

int i2cdevWriteBit(uint8_t devAddress, uint8_t memAddress,
                    uint8_t bitNum, uint8_t data)
{
    uint8_t byte;
    i2cdevReadByte(devAddress, memAddress, &byte);
    byte = (data != 0) ? (byte | (1 << bitNum)) : (byte & ~(1 << bitNum));
    return i2cdevWriteByte(devAddress, memAddress, byte);
}

int i2cdevWriteBits(uint8_t devAddress, uint8_t memAddress,
                     uint8_t bitStart, uint8_t length, uint8_t data)
{
    int status;
    uint8_t byte;

    if ((status = i2cdevReadByte(devAddress, memAddress, &byte)) == true) {
        uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
        data <<= (bitStart - length + 1); // shift data into correct position
        data &= mask;                     // zero all non-important bits in data
        byte &= ~(mask);                  // zero all important bits in existing byte
        byte |= data;                     // combine data with existing byte
        status = i2cdevWriteByte(devAddress, memAddress, byte);
    }

    return status;
}


int i2cdevWriteReg8(uint8_t devAddress, uint8_t memAddress,
                     uint16_t len, uint8_t *data)
{
    // if (xSemaphoreTake(dev->isBusFreeMutex, (TickType_t)5) == pdFALSE) {
    //     return false;
    // }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (devAddress << 1) | I2C_MASTER_WRITE, I2C_MASTER_ACK_EN);
    if (memAddress != I2CDEV_NO_MEM_ADDR) {
        i2c_master_write_byte(cmd, memAddress, I2C_MASTER_ACK_EN);
    }
    i2c_master_write(cmd, (uint8_t *)data, len, I2C_MASTER_ACK_EN);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmd, (TickType_t)5);
    i2c_cmd_link_delete(cmd);

    // xSemaphoreGive(dev->isBusFreeMutex);

#if defined CONFIG_I2CBUS_LOG_READWRITES

    if (!err) {
        char str[length * 5 + 1];

        for (size_t i = 0; i < length; i++) {
            sprintf(str + i * 5, "0x%s%X ", (data[i] < 0x10 ? "0" : ""), data[i]);
        }

        I2CBUS_LOG_RW("[port:%d, slave:0x%X] Write %d bytes to register 0x%X, data: %s",
                      port, devAddr, length, regAddr, str);
    }

#endif
#if defined CONFIG_I2CBUS_LOG_ERRORS
#if defined CONFIG_I2CBUS_LOG_READWRITES
    else {
#else

    if (err) {
#endif
        I2CBUS_LOGE("[port:%d, slave:0x%X] Failed to write %d bytes to__ register 0x%X, error: 0x%X",
                    port, devAddr, length, regAddr, err);
    }

#endif
    return !err;
}

int i2cdevWriteReg16(uint8_t devAddress, uint16_t memAddress,
                      uint16_t len, uint8_t *data)
{
    // if (xSemaphoreTake(dev->isBusFreeMutex, (TickType_t)5) == pdFALSE) {
    //     return false;
    // }

    uint8_t memAddress8[2];
    memAddress8[0] = (uint8_t)((memAddress >> 8) & 0x00FF);
    memAddress8[1] = (uint8_t)(memAddress & 0x00FF);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (devAddress << 1) | I2C_MASTER_WRITE, I2C_MASTER_ACK_EN);
    if (memAddress != I2C_NO_INTERNAL_ADDRESS) {
        i2c_master_write(cmd, memAddress8, 2, I2C_MASTER_ACK_EN);
    }
    i2c_master_write(cmd, (uint8_t *)data, len, I2C_MASTER_ACK_EN);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmd, (TickType_t)5);
    i2c_cmd_link_delete(cmd);

    // xSemaphoreGive(dev->isBusFreeMutex);
#if defined CONFIG_I2CBUS_LOG_READWRITES

    if (!err) {
        char str[length * 5 + 1];

        for (size_t i = 0; i < length; i++) {
            sprintf(str + i * 5, "0x%s%X ", (data[i] < 0x10 ? "0" : ""), data[i]);
        }

        I2CBUS_LOG_RW("[port:%d, slave:0x%X] Write %d bytes to register 0x%X, data: %s",
                      port, devAddr, length, regAddr, str);
    }

#endif

#if defined CONFIG_I2CBUS_LOG_ERRORS
#if defined CONFIG_I2CBUS_LOG_READWRITES
    else {
#else

    if (err) {
#endif
        I2CBUS_LOGE("[port:%d, slave:0x%X] Failed to write %d bytes to__ register 0x%X, error: 0x%X",
                    port, devAddr, length, regAddr, err);
    }

#endif

    return !err;
}
