#include <Wire.h>

#define I2CADDR 0x57

bool dataBaru = false;
uint8_t counter;

float v;
float y;
float yi0;
float yi1=0, yi2=0, yi3=0, yi4=0, yi5=0, yi6=0;
float i1=0, i2=0, i3=0, i4=0, i5=0, i6=0, i7=0, i8=0, i9=0, i10=0, i11=0, i12=0, i13=0, i14=0, i15=0;


float movingAverage(float value) {
  const byte nvalues = 32;             // Moving average window size

  static byte current = 0;            // Index for current value
  static byte cvalues = 0;            // Count of values read (<= nvalues)
  static float sum = 0;               // Rolling sum
  static float values[nvalues];

  sum += value;

  // If the window is full, adjust the sum by deleting the oldest value
  if (cvalues == nvalues)
    sum -= values[current];

  values[current] = value;          // Replace the oldest with the latest

  if (++current >= nvalues)
    current = 0;

  if (cvalues < nvalues)
    cvalues += 1;

  return sum/cvalues;
}

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

    // FIR Moving Average DC Removal
    v = movingAverage(ir);
    yi0 = i15 - v;
    
    
    
    // Filter FIR Window Hamming
    // LPF
    // y(n) = 0.022103306919670853009085575990866345819x(n) 
    // + 0.090845885923324792843303043810010422021x(n-1) 
    // + 0.233609522704152222649653936059621628374x(n-2) 
    // + 0.306882568905704256057020984371774829924x(n-3) 
    // + 0.233609522704152222649653936059621628374x(n-4) 
    // + 0.090845885923324792843303043810010422021x(n-5) 
    // + 0.022103306919670853009085575990866345819x(n-6)
    y = 0.022103306919670853009085575990866345819 * ir 
    + 0.090845885923324792843303043810010422021 * i1 
    + 0.233609522704152222649653936059621628374 * i2 
    + 0.306882568905704256057020984371774829924 * i3 
    + 0.233609522704152222649653936059621628374 * i4
    + 0.090845885923324792843303043810010422021 * i5
    + 0.022103306919670853009085575990866345819 * i6;
    //i6 = i5; i5 = i4; i4 = i3; i3 = i2; i2 = i1; i1 = ir; 
    i15=i14; i14=i13; i13=i12; i12=i11; i11=i10; i10=i9; i9=i8; i8=i7; i7=i6; i6=i5; i5=i4; i4=i3; i3=i2; i2=i1; i1=ir; 
    // HPF
//    wi = -0.000665543423242987200981468642879690378 * yi0 
//    - 0.002580454651955923529860204013175462023 * yi1 
//    - 0.006411713562822019332743117558948142687 * yi2 
//    + 0.991006395331781808621940399461891502142 * yi3 
//    - 0.006411713562822019332743117558948142687 * yi4
//    - 0.002580454651955923529860204013175462023 * yi5
//    - 0.000665543423242987200981468642879690378 * yi6;
//    yi6 = yi5; yi5 = yi4; yi4 = yi3; yi3 = yi2; yi2 = yi1; yi1 = yi0; 

//    // IIR DC Removal
//    wi = yi0 + 0.9 * wi1;
//    out = wi - wi1;
//    wi1 = wi;

    
    //Serial.print(ir); Serial.print(", "); Serial.print(yi0); Serial.print(", "); 
    Serial.println(y);
  }
}
