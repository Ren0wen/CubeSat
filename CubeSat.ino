/*
 * CubeSae/ I2C sensor test
 *
 * Devices on the shared bus:
 *   LSM6DS3   0x6A (or 0x6B when SDO/SA0 is pulled high)
 *   MPL115A2  0x60 *   PCF8523   0x68
 * * The sketch uses only Wire, so it does not require additional libraries.
 * Open the serial monitor at 115200 baud after programming the ATmega328.
 */
#include <Arduino.h>
#include <Wire.h>
#include <SD.h>
#include <string.h>

const uint8_t LSM6DS3_ADDRESS = 0x6A;
const uint8_t MPL115A2_ADDRESS = 0x60;
const uint8_t PCF8523_ADDRESS = 0x68;

uint8_t lsm6ds3Address = LSM6DS3_ADDRESS;
bool lsm6ds3Present = false;
uint8_t led = 0;
String wday;

File bruh;

uint8_t timedata[7] = {
  0x00, //seconds
  0x30, //minutes
  0x16, //hours
  0x11, //day
  0x00, //weekday
  0x10, //month
  0x26  //year
};

bool writeRegister(uint8_t address, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(address); Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool writeRegisters(uint8_t address, uint8_t reg, uint8_t *buffer, uint8_t count) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  for (uint8_t i = 0; i < count; ++i) Wire.write(buffer[i]);
  return Wire.endTransmission() == 0;
}

bool readRegisters(uint8_t address, uint8_t reg, uint8_t *buffer, uint8_t count) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;

  uint8_t received = Wire.requestFrom(address, count);
  if (received != count) {
    while (Wire.available()) Wire.read();
    return false;
  }
  for (uint8_t i = 0; i < count; ++i) buffer[i] = Wire.read();
  return true;
}

bool readByte(uint8_t address, uint8_t reg, uint8_t &value) {
  return readRegisters(address, reg, &value, 1);
}

int16_t signed16(uint8_t high, uint8_t low) {
  return (int16_t)(((uint16_t)high << 8) | low);
}

uint8_t bcdToDecimal(uint8_t value) {
  return ((value >> 4) * 10) + (value & 0x0F);
}

bool findLsm6ds3() {
  uint8_t whoAmI = 0;
  if (readByte(LSM6DS3_ADDRESS, 0x0F, whoAmI) && (whoAmI == 0x69 || whoAmI == 0x6A)) {
    lsm6ds3Address = LSM6DS3_ADDRESS;
    return true;
  }
  return false;
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
}

void printLsm6ds3() {
  uint8_t data[12];
  if (!lsm6ds3Present) {
    Serial.println(F("LSM6DS3 not initialized"));
    return;
  }
  if (!readRegisters(lsm6ds3Address, 0x22, data, sizeof(data))) {
    Serial.println(F("LSM6DS3 read error"));
    return;
  }

  int16_t gx = signed16(data[1], data[0]);
  int16_t gy = signed16(data[3], data[2]);
  int16_t gz = signed16(data[5], data[4]);
  int16_t ax = signed16(data[7], data[6]);
  int16_t ay = signed16(data[9], data[8]);
  int16_t az = signed16(data[11], data[10]);

  bruh.print(gx * 0.00875f, 2);
  bruh.print(F(","));
  bruh.print(gy * 0.00875f, 2);
  bruh.print(F(","));
  bruh.print(gz * 0.00875f, 2);
  bruh.print(F(","));
  bruh.print(ax * 0.000061f, 3);
  bruh.print(F(","));
  bruh.print(ay * 0.000061f, 3);
  bruh.print(F(","));
  bruh.print(az * 0.000061f, 3);
  bruh.println(F(","));

  // At the selected full scales: 8.75 mdps/LSB and 0.061 mg/LSB.
  Serial.print(F("LSM6DS3 gyro dps: "));
  Serial.print(gx * 0.00875f, 2);
  Serial.print(F(", "));
  Serial.print(gy * 0.00875f, 2);
  Serial.print(F(", "));
  Serial.print(gz * 0.00875f, 2);
  Serial.print(F(" | accel g: "));
  Serial.print(ax * 0.000061f, 3);
  Serial.print(F(", "));
  Serial.print(ay * 0.000061f, 3);
  Serial.print(F(", "));
  Serial.println(az * 0.000061f, 3);
}

bool readMpl115a2(float &pressureKpa, float &temperatureC) {
  uint8_t coefficients[8];
  uint8_t measurements[4];
  if (!readRegisters(MPL115A2_ADDRESS, 0x04, coefficients, sizeof(coefficients)) ||
      !writeRegister(MPL115A2_ADDRESS, 0x12, 0x01)) {
    return false;
  }
  delay(3);
  if (!readRegisters(MPL115A2_ADDRESS, 0x00, measurements, sizeof(measurements))) {
    return false;
  }

  int16_t a0Raw = signed16(coefficients[0], coefficients[1]);
  int16_t b1Raw = signed16(coefficients[2], coefficients[3]);
  int16_t b2Raw = signed16(coefficients[4], coefficients[5]);
  int16_t c12Raw = signed16(coefficients[6], coefficients[7]);
  float a0 = a0Raw / 8.0f;
  float b1 = b1Raw / 8192.0f;
  float b2 = b2Raw / 16384.0f;
  float c12 = c12Raw / 4194304.0f;
  float padc = ((((uint16_t)measurements[0] << 8) | measurements[1]) >> 6);
  float tadc = ((((uint16_t)measurements[2] << 8) | measurements[3]) >> 6);

  float compensated = a0 + (b1 + c12 * tadc) * padc + b2 * tadc;
  pressureKpa = compensated * (65.0f / 1023.0f) + 50.0f;
  temperatureC = 25.0f + (tadc - 472.0f) / -5.35f;
  return true;
}

void printMpl115a2() {
  float pressureKpa = 0.0f;
  float temperatureC = 0.0f;
  if (!readMpl115a2(pressureKpa, temperatureC)) {
    Serial.println(F("MPL115A2 read error"));
    return;
  }
  // print onto sd card
  bruh.print(pressureKpa, 2);
  bruh.print(F(","));
  bruh.print(temperatureC, 2);
  bruh.print(F(","));

  Serial.print(F("MPL115A2 pressure kPa: "));
  Serial.print(pressureKpa, 2);
  Serial.print(F(" | temperature C: "));
  Serial.println(temperatureC, 2);
}

void printPcf8523() {
  uint8_t clock[7];
  if (!readRegisters(PCF8523_ADDRESS, 0x03, clock, sizeof(clock))) {
    Serial.println(F("PCF8523 read error"));
    return;
  }

  //clock sets oscillator stop flag on powerup (bit 7) of seconds reg. clears after 200ms-2s
  bool clockIntegrityLost = clock[0] & 0x80;
  uint8_t seconds = bcdToDecimal(clock[0] & 0x7F);
  uint8_t minutes = bcdToDecimal(clock[1] & 0x7F);
  uint8_t hours = bcdToDecimal(clock[2] & 0x3F);
  uint8_t day = bcdToDecimal(clock[3] & 0x3F);
  uint8_t weekday = clock[4] & 0x07;
  switch (weekday) {
    case 0:
      wday = " Sun";
      break;
    case 1:
      wday = " Mon";
      break;
    case 2:
      wday = " Tue";
      break;
    case 3:
      wday = " Wed";
      break;
    case 4:
      wday = " Thu";
      break;
    case 5:
      wday = " Sat";
      break;
    default:
      wday = "";
      break;
  }
  uint8_t month = bcdToDecimal(clock[5] & 0x1F);
  uint8_t year = bcdToDecimal(clock[6]);

  Serial.print(F("PCF8523 time 20"));
  if (year < 10) Serial.print('0');
  Serial.print(year);
  Serial.print('-');
  if (month < 10) Serial.print('0');
  Serial.print(month);
  Serial.print('-');
  if (day < 10) Serial.print('0');
  Serial.print(day);
  Serial.print(' ');
  if (hours < 10) Serial.print('0');
  Serial.print(hours);
  Serial.print(':');
  if (minutes < 10) Serial.print('0');
  Serial.print(minutes);
  Serial.print(':');
  if (seconds < 10) Serial.print('0');
  Serial.print(seconds);
  Serial.print(F(" | weekday: "));
  Serial.print(wday);
  if (clockIntegrityLost) Serial.print(F(" | WARNING: clock integrity lost"));
  Serial.println();

//date
  if (year < 10) bruh.print('0');
  bruh.print(year);
  bruh.print('-');
  if (month < 10) bruh.print('0');
  bruh.print(month);
  bruh.print('-');
  if (day < 10) bruh.print('0');
  bruh.print(day);
  bruh.print(",");
//dotw
  bruh.print(wday);
  bruh.print(F(","));
//time
  if (hours < 10) bruh.print('0');
  bruh.print(hours);
  bruh.print(':');
  if (minutes < 10) bruh.print('0');
  bruh.print(minutes);
  bruh.print(':');
  if (seconds < 10) bruh.print('0');
  bruh.print(seconds);
  bruh.print(F(","));
}

bool clearPcf8523OsFlag() {
  uint8_t seconds;
  if (!readByte(PCF8523_ADDRESS, 0x03, seconds)) return false;
  if ((seconds & 0x80) == 0) return true;

  // datasheeyt specifies that the OS flag is set at powerup while the crystal starts. give it time to stabilize, then rewrite seconds with OS = 0.
  
  delay(2000);
  if (!readByte(PCF8523_ADDRESS, 0x03, seconds)) return false;
  if ((seconds & 0x80) && !writeRegister(PCF8523_ADDRESS, 0x03, seconds & 0x7F)) {
    return false;
  }
  if (!readByte(PCF8523_ADDRESS, 0x03, seconds)) return false;
  return (seconds & 0x80) == 0;
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(A0, OUTPUT);
  pinMode(10, OUTPUT);

  char filename[15];
  uint8_t filenumber = 0;

  if(!SD.begin(10)) Serial.println("sd card reader initialization failed");
  else { 

  //csv file
  do { snprintf(filename,sizeof(filename), "bruh%03u.csv", filenumber++); } 
  while (SD.exists(filename));
    bruh = SD.open(filename, FILE_WRITE);
  }

  if (bruh)
    Serial.println("successfully opened test.txt");
  else
    Serial.println("unsuccessfully opened test.txt");

  //header 
  bruh.println("date,dotw,time,pressure (kPa),temperature (C),gx,gy,gz,ax,ay,az,");

  Serial.println(F("CubeSat I2C sensor test"));
  Serial.println(F("SDA/SCL bus scan:"));

  bool lsmOk = initLsm6ds3();
  uint8_t scratch = 0;
  bool mplOk = readByte(MPL115A2_ADDRESS, 0x00, scratch);
  bool pcfOk = readByte(PCF8523_ADDRESS, 0x03, scratch);
  bool pcfClockOk = pcfOk && clearPcf8523OsFlag();

  Serial.print(F("LSM6DS3: "));
  Serial.println(lsmOk ? F("FOUND") : F("NOT FOUND"));
  if (lsmOk == true) {
    led++;
  }
  Serial.print(F("MPL115A2: "));
  Serial.println(mplOk ? F("FOUND") : F("NOT FOUND"));
  if (mplOk == true) {
    led++;
  }
  Serial.print(F("PCF8523: "));
  if (!pcfOk) {
    Serial.println(F("NOT FOUND"));
  } else if (!pcfClockOk) {
    Serial.println(F("FOUND, oscillator not running or clock not initialized"));
  } else {
    led++;
    Serial.println(F("FOUND, clock oscillator running"));
    //set the time and date
    writeRegisters(PCF8523_ADDRESS, 0x03, timedata, sizeof(timedata));
  }
}

void loop() {
  for (int i = 0; i < 5; i++) {
  printPcf8523();
  printMpl115a2();
  printLsm6ds3();
  Serial.println();

  //led to display working modules
  switch (led) {
    case 0:
      break;
    case 1:
      analogWrite(A0, 1023);
      delay(100);
      analogWrite(A0, 0);
      delay(900);
      break;
    case 2:
      analogWrite(A0, 1023);
      delay(100);
      analogWrite(A0, 0);
      delay(100);
      analogWrite(A0, 1023);
      delay(100);
      analogWrite(A0, 0);
      delay(600);
      break;
    case 3:
      analogWrite(A0, 1023);
      delay(1000);
      break;
    default:
      analogWrite(A0, 0);
      delay(1000);
      break;

  }
}
 bruh.close();
 while(1)
{}
}
