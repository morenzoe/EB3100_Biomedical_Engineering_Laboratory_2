#include <Wire.h>

#define I2CADDR 0x57

bool dataBaru = false;
uint8_t counter;
float yi0, yi1, yi2, i1, i2;
float yr0, yr1, yr2, r1, r2;
bool peakFound = false; bool turun = false;
int t_prev = 0; int t_curr = 0; int t_delta = 0; float y_prev = 0; float y_curr = 0;
float treshold = 300;
float bpm = 0; int bpmIdx = 0;


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

// Moving Average
float movingAverage(float value) {
  const byte nvalues = 10;             // Moving average window size

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


void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(2, INPUT);
  registerWrite(0x06, B01000000); // Reset
  registerWrite(0x01, B00010000); // SpO2 Data Ready Interrupt
  registerWrite(0x06, B00000011); // SpO2 mode
  registerWrite(0x07, B00000111); // 100 sps ADC, 1600 us pulse width
  //registerWrite(0x09, B01110111); // 24.0 mA RED, 24.0 mA IR
  registerWrite(0x09, B10011101); // 30.6 mA RED, 37.0 mA IR
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
    else {                        // y[n]= y[n-2] + x[n] - 1.929*x[n-1] + 0.997*x[n-2]
      yi0 = (1.692*yi1) - (0.742*yi2) + ir - i2;
      yi2 = yi1; i2 = i1; yi1 = yi0; i1 = ir;
      yr0 = (1.692*yr1) - (0.742*yr2) + red - r2;
      yr2 = yr1; r2 = r1; yr1 = yr0; r1 = red;
      //Serial.print(-yi0); Serial.print('\t'); Serial.print(treshold); Serial.print('\t');  Serial.println(-yr0);
    }
  

  //HR Detection
   y_curr = (-yi0);
   //Serial.print(peakFound); Serial.print('\t'); Serial.print(treshold); Serial.print('\t'); Serial.print(y_prev); Serial.print('\t'); Serial.println(y_curr);

   
   if (y_curr < treshold){
    //Serial.println("di bawah threshold");
    //Di bawah treshold dan turun
    if (y_curr < y_prev){ 
      turun = true;
      peakFound = false;
    }
    //Di bawah treshold dan naik
    else{
      turun = false;
      //Serial.println(turun);
    }
    y_prev = y_curr;
   }
   
   else if ((y_curr >= treshold) && (peakFound == false)){
    //Serial.println("Masuk treshold");
      if (y_curr < y_prev){
        //Serial.println("Peak Found!");
        t_curr = millis();
        y_prev = y_curr;
        peakFound = true;
        bpmIdx++;
        turun = true;
        //Serial.println(turun);

        
        //itung bpm
        t_delta = t_curr - t_prev;
        t_prev = t_curr;
        bpm = movingAverage(60000/t_delta);
        
        if (bpmIdx >= 15){
          Serial.print("BPM: ");
          Serial.println(bpm);
        }
        else{
          Serial.println("Please wait, still processing");
        }
        
        
      }
      else{
        y_prev = y_curr;
        //Serial.println("Gaketemu :(");
      }
   }

   else if ((y_curr >= treshold) && (peakFound == true)){
    y_prev = y_curr;
    //Serial.println("Masuk treshold tapi dah lewat");
    
   }
 
}

}
