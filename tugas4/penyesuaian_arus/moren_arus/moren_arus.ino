#include <Wire.h>

#define I2CADDR 0x57

bool dataBaru = false;

void registerWrite(uint8_t regaddr, uint8_t regdata) {
  Wire.beginTransmission(I2CADDR);
  Wire.write(regaddr);
  Wire.write(regdata);
  Wire.endTransmission();
}

uint8_t registerRead(uint8_t regaddr) {
  Wire.beginTransmission(I2CADDR);
  
  Wire.write(regaddr);
  Wire.endTransmission(0);
  Wire.requestFrom(I2CADDR, 1);
  return Wire.read();
}

void dataRead(uint8_t *FIFOdata) {
  Wire.beginTransmission(I2CADDR);
  Wire.write(0x05);
  Wire.endTransmission(0);
  Wire.requestFrom(I2CADDR, 4);
  FIFOdata[0] = Wire.read();
  FIFOdata[1] = Wire.read();
  FIFOdata[2] = Wire.read();
  FIFOdata[3] = Wire.read();
}

void adaDataBaru() {
  dataBaru = true;
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(2, INPUT);
  registerWrite(0x06, B01000000); // Reset
  registerWrite(0x01, B00010000); // SpO2 Data Ready Interrupt
  registerWrite(0x06, B00000011); // SpO2 mode
  registerWrite(0x07, B00000111); // 100 sps ADC, 1600 us pulse width
  registerWrite(0x09, B10101000); // 33.8 mA RED, 27.1 mA IR
  attachInterrupt(digitalPinToInterrupt(2), adaDataBaru, FALLING );
  registerRead(0x00);
}

void loop() {
  uint8_t data[4];
  uint16_t ir, red;
  //byte toRead = registerRead(0x02) - registerRead(0x04);
  //if (toRead) {
  if (dataBaru) {
    dataBaru = false;
    dataRead(data);
    ir  = (data[0] << 8) | data[1];
    red = (data[2] << 8) | data[3];
    Serial.print(ir); Serial.print(", "); Serial.println(red);
  }
}
