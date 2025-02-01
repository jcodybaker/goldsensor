#include "VL53L0X.h"

#define ADDRESS_DEFAULT 0b0101001

// Write an 8-bit register
void VL53L0X::writeReg(uint8_t reg, uint8_t value)
{
  ESP_LOGI("VL53L0X", "writing 8-bit to reg %x", reg);
  i2c_master_write_read_device(I2C_NUM_0, ADDRESS_DEFAULT)

  bus->writeRegister8(reg, value);
  // // bus->beginTransmission(address);
  // bus->start(false);
  // bus->write(reg);
  // bus->write(value);
  // last_status = bus->stop();
}

// Write a 16-bit register
void VL53L0X::writeReg16Bit(uint8_t reg, uint16_t value)
{
  ESP_LOGI("VL53L0X", "writing 16-bit to reg %x", reg);
  uint8_t buf[2] = {
    (uint8_t)((value &  0x0000FF00) >> 8),
    (uint8_t) (value &  0x000000FF),
  };
  bus->writeRegister(reg, buf, 2);
  // bus->start(address, false, frequency);
  // bus->write(reg);
  // bus->write((uint8_t)); // value high byte
  // bus->write((uint8_t)(value)); // value low byte
  // last_status = bus->stop();
}

// Write a 32-bit register
void VL53L0X::writeReg32Bit(uint8_t reg, uint32_t value)
{
  ESP_LOGI("VL53L0X", "writing 32-bit to reg %x", reg);
  uint8_t buf[4] = {
    // Split 32-bit word into MS ... LS bytes
    (uint8_t) (value >> 24),
    (uint8_t)((value &  0x00FF0000) >> 16),
    (uint8_t)((value &  0x0000FF00) >> 8),
    (uint8_t) (value &  0x000000FF),
  };
  bus->writeRegister(reg, buf, 4);
  // bus->start(address, false, frequency);
  // bus->write(reg);
  // bus->write((uint8_t)(value >> 24)); // value highest byte
  // bus->write((uint8_t)(value >> 16));
  // bus->write((uint8_t)(value >>  8));
  // bus->write((uint8_t)(value));       // value lowest byte
  // last_status = bus->stop();
}

// Read an 8-bit register
uint8_t VL53L0X::readReg(uint8_t reg)
{
  ESP_LOGI("VL53L0X", "reading 8-bit from reg %x, bus %lx", reg, (uint32_t)bus);
  uint8_t v = bus->readRegister8(reg);
  ESP_LOGI("VL53L0X", "finished reading %x 8-bit from reg %x", v, reg );
  return v;
  // uint8_t value;

  // bus->start(address, true, frequency);
  // bus->write(reg);
  // last_status = bus->stop();

  // bus->requestFrom(address, (uint8_t)1);
  // value = bus->read();

  // return value;
}

// Read a 16-bit register
uint16_t VL53L0X::readReg16Bit(uint8_t reg)
{
  ESP_LOGI("VL53L0X", "reading 16-bit from reg %x", reg);
  uint16_t value;
  uint8_t buf[2];
  bus->readRegister(reg, buf, 2);

  value = ((uint32_t)buf[0]<<8) + (uint32_t)buf[1];

  return value;
}

// Read a 32-bit register
uint32_t VL53L0X::readReg32Bit(uint8_t reg)
{
  ESP_LOGI("VL53L0X", "reading 32-bit from reg %x", reg);
  uint32_t value;
  uint8_t buf[4];
  bus->readRegister(reg, buf, 4);
  value = ((uint32_t)buf[0]<<24) + ((uint32_t)buf[1]<<16) + ((uint32_t)buf[2]<<8) + (uint32_t)buf[3];
  return value;
}

// Write an arbitrary number of bytes from the given array to the sensor,
// starting at the given register
void VL53L0X::writeMulti(uint8_t reg, uint8_t const * src, uint8_t count)
{
  ESP_LOGI("VL53L0X", "writing %d bytes to reg %x", count, reg);
  last_status = bus->writeRegister(reg, src, count);
}

// Read an arbitrary number of bytes from the sensor, starting at the given
// register, into the given array
void VL53L0X::readMulti(uint8_t reg, uint8_t * dst, uint8_t count)
{
  ESP_LOGI("VL53L0X", "reading %d bytes from reg %x", count, reg);
  last_status = bus->readRegister(reg, dst, count);
}


// #include "VL53L0X.h"

// // Write an 8-bit register
// void VL53L0X::writeReg(uint8_t reg, uint8_t value)
// {
//   ESP_LOGI("VL53L0X", "writing 8-bit to reg %x", reg);
//   bus->writeRegister8(reg, value);
//   // // bus->beginTransmission(address);
//   // bus->start(false);
//   // bus->write(reg);
//   // bus->write(value);
//   // last_status = bus->stop();
// }

// // Write a 16-bit register
// void VL53L0X::writeReg16Bit(uint8_t reg, uint16_t value)
// {
//   ESP_LOGI("VL53L0X", "writing 16-bit to reg %x", reg);
//   uint8_t buf[2] = {
//     (uint8_t)((value &  0x0000FF00) >> 8),
//     (uint8_t) (value &  0x000000FF),
//   };
//   bus->writeRegister(reg, buf, 2);
//   // bus->start(address, false, frequency);
//   // bus->write(reg);
//   // bus->write((uint8_t)); // value high byte
//   // bus->write((uint8_t)(value)); // value low byte
//   // last_status = bus->stop();
// }

// // Write a 32-bit register
// void VL53L0X::writeReg32Bit(uint8_t reg, uint32_t value)
// {
//   ESP_LOGI("VL53L0X", "writing 32-bit to reg %x", reg);
//   uint8_t buf[4] = {
//     // Split 32-bit word into MS ... LS bytes
//     (uint8_t) (value >> 24),
//     (uint8_t)((value &  0x00FF0000) >> 16),
//     (uint8_t)((value &  0x0000FF00) >> 8),
//     (uint8_t) (value &  0x000000FF),
//   };
//   bus->writeRegister(reg, buf, 4);
//   // bus->start(address, false, frequency);
//   // bus->write(reg);
//   // bus->write((uint8_t)(value >> 24)); // value highest byte
//   // bus->write((uint8_t)(value >> 16));
//   // bus->write((uint8_t)(value >>  8));
//   // bus->write((uint8_t)(value));       // value lowest byte
//   // last_status = bus->stop();
// }

// // Read an 8-bit register
// uint8_t VL53L0X::readReg(uint8_t reg)
// {
//   ESP_LOGI("VL53L0X", "reading 8-bit from reg %x, bus %lx", reg, (uint32_t)bus);
//   uint8_t v = bus->readRegister8(reg);
//   ESP_LOGI("VL53L0X", "finished reading %x 8-bit from reg %x", v, reg );
//   return v;
//   // uint8_t value;

//   // bus->start(address, true, frequency);
//   // bus->write(reg);
//   // last_status = bus->stop();

//   // bus->requestFrom(address, (uint8_t)1);
//   // value = bus->read();

//   // return value;
// }

// // Read a 16-bit register
// uint16_t VL53L0X::readReg16Bit(uint8_t reg)
// {
//   ESP_LOGI("VL53L0X", "reading 16-bit from reg %x", reg);
//   uint16_t value;
//   uint8_t buf[2];
//   bus->readRegister(reg, buf, 2);

//   value = ((uint32_t)buf[0]<<8) + (uint32_t)buf[1];

//   return value;
// }

// // Read a 32-bit register
// uint32_t VL53L0X::readReg32Bit(uint8_t reg)
// {
//   ESP_LOGI("VL53L0X", "reading 32-bit from reg %x", reg);
//   uint32_t value;
//   uint8_t buf[4];
//   bus->readRegister(reg, buf, 4);
//   value = ((uint32_t)buf[0]<<24) + ((uint32_t)buf[1]<<16) + ((uint32_t)buf[2]<<8) + (uint32_t)buf[3];
//   return value;
// }

// // Write an arbitrary number of bytes from the given array to the sensor,
// // starting at the given register
// void VL53L0X::writeMulti(uint8_t reg, uint8_t const * src, uint8_t count)
// {
//   ESP_LOGI("VL53L0X", "writing %d bytes to reg %x", count, reg);
//   last_status = bus->writeRegister(reg, src, count);
// }

// // Read an arbitrary number of bytes from the sensor, starting at the given
// // register, into the given array
// void VL53L0X::readMulti(uint8_t reg, uint8_t * dst, uint8_t count)
// {
//   ESP_LOGI("VL53L0X", "reading %d bytes from reg %x", count, reg);
//   last_status = bus->readRegister(reg, dst, count);
// }


