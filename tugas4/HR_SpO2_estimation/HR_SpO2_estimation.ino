/*EB3100 Praktikum Teknik Biomedis 2
 *Modul             : 4 - Estimasi HR dan SpO2
 *Hari dan Tanggal  : Jumat, 25 November 2022
 *Nama (NIM) 1      : Eraraya Morenzo Muten (18320003)
 *Nama (NIM) 2      : Kayyisa Zahratulfirdaus (18320011)
 *Nama (NIM) 3      : Rahmat Yasin (18319001)
 *Asisten (NIM)     : Syifa Kushirayati )18319037) 
 *Nama File         : HR_SpO2_estimation
 *Deskripsi         : Program akan melakukan pengolahan sinyal dengan LPF FIR Hamming Window dan DC removal FIR filter
 *                    dengan moving average. Heart rate kemudian dihitung dengan mendeteksi dan mengukur waktu antar puncak R
 *                    gelombang PPG. SpO2 kemudian dihitung dengan perbandingan logaritma dari RMS merah dan inframerah.
 */

#include <Wire.h>
#define I2CADDR 0x57

// Deklarasi Variabel
// Pembacaan data
bool dataBaru = false;

// Memori filter sinyal inframerah
float vi;
float wi;
float yi0;
float yi1=0, yi2=0, yi3=0, yi4=0, yi5=0, yi6=0, yi7=0, yi8=0, yi9=0, yi10=0, yi11=0, yi12=0, yi13=0, yi14=0, yi15=0;
float i1=0, i2=0, i3=0, i4=0, i5=0, i6=0;

// Memori filter sinyal merah
float vr;
float wr;
float yr0;
float yr1=0, yr2=0, yr3=0, yr4=0, yr5=0, yr6=0, yr7=0, yr8=0, yr9=0, yr10=0, yr11=0, yr12=0, yr13=0, yr14=0, yr15=0;
float r1=0, r2=0, r3=0, r4=0, r5=0, r6=0;

bool peakFound = false; bool turun = false;
int t_prev = 0; int t_curr = 0; int t_delta = 0; float y_prev = 0; float y_curr = 0;
float treshold = 80;
float bpm = 0; int bpmIdx = 0;

float irACsum;
float redACsum;
uint8_t counter;
float R;
float spo2;
float spo2_m;

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

float movingAverageIR(float val) {
  const byte win_size = 32;   // ukuran window
  static byte val_idx = 0;    // index nilai terbaru
  static byte num = 0;        // jumlah elemen di dalam array yang sudah terisi
  static float sum = 0;       // penjumlahan seluruh elemen
  static float win[win_size]; // array untuk menyimpan nilai yang berada di dalam window
  
  // menjumlahkan nilai terbaru ke variabel jumlah
  sum += val; 

  // secara FIFO, nilai yang pertama masuk akan dikeluarkan dari array
  if (num == win_size){
    // penjumlahan elemen array dikurangi dengan nilai yang dikeluarkan
    sum -= win[val_idx];
  }
  
  // mengganti elemen di index nilai sekarang dengan nilai terbaru
  win[val_idx] = val;        

  // menambah index nilai terbaru
  val_idx += 1;
  
  // mereset index jika index sudah melebihi ukuran window
  if (val_idx >= win_size){
    val_idxt = 0;
  }
  
  // jika jumlah nilai yang disimpan di array kurang dari ukuran window
  if (num < win_size){
    // tambahkan jumlah elemen tersebut sehingga pembagian sesuai 
    num += 1;
  }
  
  return sum/num;
}

float movingAverageRED(float val) {
  const byte win_size = 32;   // ukuran window
  static byte val_idx = 0;    // index nilai terbaru
  static byte num = 0;        // jumlah elemen di dalam array yang sudah terisi
  static float sum = 0;       // penjumlahan seluruh elemen
  static float win[win_size]; // array untuk menyimpan nilai yang berada di dalam window
  
  // menjumlahkan nilai terbaru ke variabel jumlah
  sum += val; 

  // secara FIFO, nilai yang pertama masuk akan dikeluarkan dari array
  if (num == win_size){
    // penjumlahan elemen array dikurangi dengan nilai yang dikeluarkan
    sum -= win[val_idx];
  }
  
  // mengganti elemen di index nilai sekarang dengan nilai terbaru
  win[val_idx] = val;        

  // menambah index nilai terbaru
  val_idx += 1;
  
  // mereset index jika index sudah melebihi ukuran window
  if (val_idx >= win_size){
    val_idxt = 0;
  }
  
  // jika jumlah nilai yang disimpan di array kurang dari ukuran window
  if (num < win_size){
    // tambahkan jumlah elemen tersebut sehingga pembagian sesuai 
    num += 1;
  }
  
  return sum/num;
}

float movingAverageHR(float val) {
  const byte win_size = 10;   // ukuran window
  static byte val_idx = 0;    // index nilai terbaru
  static byte num = 0;        // jumlah elemen di dalam array yang sudah terisi
  static float sum = 0;       // penjumlahan seluruh elemen
  static float win[win_size]; // array untuk menyimpan nilai yang berada di dalam window
  
  // menjumlahkan nilai terbaru ke variabel jumlah
  sum += val; 

  // secara FIFO, nilai yang pertama masuk akan dikeluarkan dari array
  if (num == win_size){
    // penjumlahan elemen array dikurangi dengan nilai yang dikeluarkan
    sum -= win[val_idx];
  }
  
  // mengganti elemen di index nilai sekarang dengan nilai terbaru
  win[val_idx] = val;        

  // menambah index nilai terbaru
  val_idx += 1;
  
  // mereset index jika index sudah melebihi ukuran window
  if (val_idx >= win_size){
    val_idxt = 0;
  }
  
  // jika jumlah nilai yang disimpan di array kurang dari ukuran window
  if (num < win_size){
    // tambahkan jumlah elemen tersebut sehingga pembagian sesuai 
    num += 1;
  }
  
  return sum/num;
}

float movingAverageSpO2(float val) {
  const byte win_size = 10;   // ukuran window
  static byte val_idx = 0;    // index nilai terbaru
  static byte num = 0;        // jumlah elemen di dalam array yang sudah terisi
  static float sum = 0;       // penjumlahan seluruh elemen
  static float win[win_size]; // array untuk menyimpan nilai yang berada di dalam window
  
  // menjumlahkan nilai terbaru ke variabel jumlah
  sum += val; 

  // secara FIFO, nilai yang pertama masuk akan dikeluarkan dari array
  if (num == win_size){
    // penjumlahan elemen array dikurangi dengan nilai yang dikeluarkan
    sum -= win[val_idx];
  }
  
  // mengganti elemen di index nilai sekarang dengan nilai terbaru
  win[val_idx] = val;        

  // menambah index nilai terbaru
  val_idx += 1;
  
  // mereset index jika index sudah melebihi ukuran window
  if (val_idx >= win_size){
    val_idxt = 0;
  }
  
  // jika jumlah nilai yang disimpan di array kurang dari ukuran window
  if (num < win_size){
    // tambahkan jumlah elemen tersebut sehingga pembagian sesuai 
    num += 1;
  }
  
  return sum/num;
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

    // Low Pass Filter FIR Window Hamming
    yi0 = 0.022103306919670853009085575990866345819 * ir 
    + 0.090845885923324792843303043810010422021 * i1 
    + 0.233609522704152222649653936059621628374 * i2 
    + 0.306882568905704256057020984371774829924 * i3 
    + 0.233609522704152222649653936059621628374 * i4
    + 0.090845885923324792843303043810010422021 * i5
    + 0.022103306919670853009085575990866345819 * i6;
    i6 = i5; i5 = i4; i4 = i3; i3 = i2; i2 = i1; i1 = ir; 

    // FIR Moving Average DC Removal
    vi = movingAverage_ir(yi0);
    wi = yi15 - vi;
    yi15=yi14; yi14=yi13; yi13=yi12; yi12=yi11; yi11=yi10; yi10=yi9; yi9=yi8; yi8=yi7; yi7=yi6; yi6=yi5; yi5=yi4; yi4=yi3; yi3=yi2; yi2=yi1; yi1=yi0;

    // Low Pass Filter FIR Window Hamming 
    yr0 = 0.022103306919670853009085575990866345819 * red 
    + 0.090845885923324792843303043810010422021 * r1 
    + 0.233609522704152222649653936059621628374 * r2 
    + 0.306882568905704256057020984371774829924 * r3 
    + 0.233609522704152222649653936059621628374 * r4
    + 0.090845885923324792843303043810010422021 * r5
    + 0.022103306919670853009085575990866345819 * r6;
    r6 = r5; r5 = r4; r4 = r3; r3 = r2; r2 = r1; r1 = red; 

    // FIR Moving Average DC Removal
    vr = movingAverage_red(yr0);
    wr = yr15 - vr;
    yr15=yr14; yr14=yr13; yr13=yr12; yr12=yr11; yr11=yr10; yr10=yr9; yr9=yr8; yr8=yr7; yr7=yr6; yr6=yr5; yr5=yr4; yr4=yr3; yr3=yr2; yr2=yr1; yr1=yr0; 
    
    //Serial.print(yi0); Serial.print(", "); Serial.println(yr0);
    //Serial.print(ir-30000); Serial.print(", "); Serial.println(wi);

      //HR Detection
   y_curr = (wi);
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
          Serial.print("Heart Rate: ");
          Serial.println(int(bpm));
//          Serial.print("Ratio: ");
//          Serial.println(R);
          Serial.print("SPO2: ");
          Serial.println(int(spo2));//Serial.println(" % tanpa");
          //Serial.print(spo2_m);Serial.println(" % moving");
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

    //SpO2 calculation
    irACsum += wi*wi;
    redACsum += wr*wr;
    counter++;

    if (peakFound){
      R = log(sqrt(redACsum/counter)) / log(sqrt(irACsum/counter));
      spo2 = 110.0 - 18.0 * R;
      spo2_m = movingAverage_spo2(spo2);
      
    }

    if (bpmIdx>4){
      irACsum = 0;
      redACsum = 0;
      counter = 0;
    }
    
    }
    
  }
