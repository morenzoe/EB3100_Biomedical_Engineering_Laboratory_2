#include <Wire.h>

#define I2CADDR 0x57

bool dataBaru = false;
uint8_t counter;
float wi, yi0, yi1=0, yi2=0, yi3=0, yi4=0, yi5=0, yi6=0, i1=0, i2=0, i3=0, i4=0, i5=0, i6=0;
float wr, yr0, yr1=0, yr2=0, yr3=0, yr4=0, yr5=0, yr6=0, r1=0, r2=0, r3=0, r4=0, r5=0, r6=0;

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
  registerWrite(0x09, B10000111); // 27.1 mA RED, 24.0 mA IR
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

    // Filter FIR Window Hamming
    // LPF
    // y(n) = 0.022103306919670853009085575990866345819x(n) 
    // + 0.090845885923324792843303043810010422021x(n-1) 
    // + 0.233609522704152222649653936059621628374x(n-2) 
    // + 0.306882568905704256057020984371774829924x(n-3) 
    // + 0.233609522704152222649653936059621628374x(n-4) 
    // + 0.090845885923324792843303043810010422021x(n-5) 
    // + 0.022103306919670853009085575990866345819x(n-6)
    yi0 = 0.022103306919670853009085575990866345819 * ir 
    + 0.090845885923324792843303043810010422021 * i1 
    + 0.233609522704152222649653936059621628374 * i2 
    + 0.306882568905704256057020984371774829924 * i3 
    + 0.233609522704152222649653936059621628374 * i4
    + 0.090845885923324792843303043810010422021 * i5
    + 0.022103306919670853009085575990866345819 * i6;
    i6 = i5; i5 = i4; i4 = i3; i3 = i2; i2 = i1; i1 = ir; 
    // HPF
    wi = -0.000665543423242987200981468642879690378 * yi0 
    - 0.002580454651955923529860204013175462023 * yi1 
    - 0.006411713562822019332743117558948142687 * yi2 
    + 0.991006395331781808621940399461891502142 * yi3 
    - 0.006411713562822019332743117558948142687 * yi4
    - 0.002580454651955923529860204013175462023 * yi5
    - 0.000665543423242987200981468642879690378 * yi6;
    yi6 = yi5; yi5 = yi4; yi4 = yi3; yi3 = yi2; yi2 = yi1; yi1 = yi0; 
    
    Serial.print(ir); Serial.print(", "); Serial.print(yi0); Serial.print(", "); Serial.println(wi);
  }
}
