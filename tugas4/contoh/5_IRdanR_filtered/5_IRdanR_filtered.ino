#include <Wire.h>

#define I2CADDR 0x57

bool dataBaru = false;
uint8_t counter;
float yi0, yi1, yi2, i1, i2;
float yr0, yr1, yr2, r1, r2;

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
  registerWrite(0x07, B00000011); // 50 sps ADC, 1600 us pulse width
  //registerWrite(0x09, B01110111); // 24.0 mA RED, 24.0 mA IR
  registerWrite(0x09, B10011011); // 30.6 mA RED, 37.0 mA IR
  attachInterrupt(digitalPinToInterrupt(2), adaDataBaru, FALLING );
  registerRead(0x00);
  counter = 0;
}

void loop() {
  uint8_t data[4];
  uint16_t ir, red;

  if (dataBaru) {
    dataBaru = false;
    dataRead(data);
    ir  = (data[0] << 8) | data[1];
    red = (data[2] << 8) | data[3];
    if (counter < 2) {
      yi0 = 0; yi1 = 0; yi2 = 0; i1 = ir;  i2 = i1;
      yr0 = 0; yr1 = 0; yr2 = 0; r1 = red; r2 = r1;
      counter++;
    }
    else {                        // y[n]= 0.9025*y[n-2] + x[n] - x[n-2]
      yi0 = 0.9025 * yi2 + ir  - i2;
      yi2 = yi1; i2 = i1; yi1 = yi0; i1 = ir;
      yr0 = 0.9025 * yr2 + red - r2;
      yr2 = yr1; r2 = r1; yr1 = yr0; r1 = red;
      Serial.print(-yi0); Serial.print('\t'); Serial.println(-yr0);
    }
  }
}
