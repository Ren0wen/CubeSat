#include <Arduino.h>
#include <Wire.h>

/*
 * CubeSae/ I2C sensor test
 *
 * Devices on the shared bus:
 *   LSM6DS3   0x6A (or 0x6B when SDO/SA0 is pulled high)
 *   MPL115A2  0x60
 *   PCF8523   0x68
 *
 * The sketch uses only Wire, so it does not require additional libraries.
 * Open the serial monitor at 115200 baud after programming the ATmega328.
 *
 */

const uint8_t LSM6DS3_ADDRESS_LOW = 0x6A;
const uint8_t LSM6DS3_ADDRESS_HIGH = 0x6B;
const uint8_t MPL115A2_ADDRESS = 0x60;
const uint8_t PCF8523_ADDRESS = 0x68;

bool readByte(uint8_t address, uint8_t reg, uint8_t &value) {
  return readRegisters(address, reg, &value, 1);
}

bool findLSM() {
  uint8_t whoami = 0;  
  //read byte from who am i ID (0x0f) and store in whoami
  //the lsm has an spi and i2c communication protocol, we aren't sure if low a
  if (readByte(LSM6DS3_ADDRESS_LOW, 0x0F, whoami) && (whoami == 0x69) { 
    lsm6ds3Address = LSM6DS3_ADDRESS_LOW;
    return true;
  }
  if (readByte(LSM6DS3_ADDRESS_HIGH, 0x0F, whoAmI) && (whoAmI == 0x69)) {
    lsm6ds3Address = LSM6DS3_ADDRESS_HIGH;
    return true;
  }
  return false;
  }
}

bool initLsm6ds3() {
  if (!findLsm6ds3()) return false;
  // Accelerometer: 104 Hz, +/-2 g. Gyroscope: 104 Hz, +/-245 dps.
  // BDU prevents a multi-byte sample from changing halfway through a read;
  // IF_INC enables sequential reads from the output registers.
  lsm6ds3Present = writeRegister(lsm6ds3Address, 0x10, 0x40) &&
                   writeRegister(lsm6ds3Address, 0x11, 0x40) &&
                   writeRegister(lsm6ds3Address, 0x12, 0x44);
  return lsm6ds3Present;
