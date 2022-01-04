// https://wiki.analog.com/resources/eval/user-guides/eval-adicup360/hardware/adxl355  (Hardware descrption)

#define ADXL_address (0x1D) // 0x53


/*const int XDATA3 = 0x08;
  const int XDATA2 = 0x09;
  const int XDATA1 = 0x0A;
  const int YDATA3 = 0x0B;
  const int YDATA2 = 0x0C;
  const int YDATA1 = 0x0D;
  const int ZDATA3 = 0x0E;
  const int ZDATA2 = 0x0F;
  const int ZDATA1 = 0x10;


  const int TEMP1 = 0x07;
  const int TEMP2 = 0x06;
  const int range = 0x2C;  // range, interrupt_polarity y i2c_speed  01000000
  const int self_test = 0x2E;
  const int reset_acc = 0x2F; // 0x52 reinicia el accelerometro sin necesitar desconectarlo
  const int power_control = 0x2D; // 000XXXXX
  const int filter = 0x28; // definicion de frecuencia de muestreo en base a a velocidad de I2C,  00110000 (125Hz) 8ms de periodo en fast mode


  byte value;

  const int READ_BYTE = 0x01;
  const int WRITE_BYTE = 0x00;*/

void iniciar_adxl() {
  Wire.beginTransmission(ADXL_address);
  Wire.write(power_control);
  Wire.write(0);
  Wire.endTransmission();
}


void set_range_2g() {
  Wire.beginTransmission(ADXL_address);
  Wire.write(range);
  Wire.write(64);
  Wire.endTransmission();
}

void set_sample_rate() {
  Wire.beginTransmission(ADXL_address);
  Wire.write(filter);
  Wire.write(0x01010000);  // 00000000 sample rate de 125hz
  Wire.endTransmission();
}



float *read_acc(float *mediciones) {
  
  uint32_t D1X;
  uint32_t D2X;
  uint32_t D3X;
  uint32_t D1Y;
  uint32_t D2Y;
  uint32_t D3Y;
  uint32_t D1Z;
  uint32_t D2Z;
  uint32_t D3Z;

  //millis_anterior = micros();
  Wire.beginTransmission(ADXL_address); // Begin transmission to the Sensor
  Wire.write(XDATA3);
  Wire.endTransmission();
  Wire.requestFrom(ADXL_address, 9);
  if (Wire.available()) {
    D3X = Wire.read();
    D2X = Wire.read();
    D1X = Wire.read();
    D3Y = Wire.read();
    D2Y = Wire.read();
    D1Y = Wire.read();
    D3Z = Wire.read();
    D2Z = Wire.read();
    D1Z = Wire.read();
  }

  // Ends the transmission and transmits the data from the two registers


  uint32_t v1 = D3X;     // Ternary operator - if wire is available then read value of v1 else write 0
  uint32_t v2 = D2X;     // Ternary operator - if wire is available then read value of v2 else write 0
  uint32_t v3 = D1X;     // Ternary operator - if wire is available then read value of v3 else write 0
  uint32_t d = (v1 << 16) | (v2 << 8) | v3;      // Bit shift v1 16 bits to the left, v2 8 bits to the left
  d = d >> 4;                                 // Bit shift data 4 bits to the right
  float value = (float)d;                        // value = data
  mediciones[0] = d > 0X7FFFF ? value - 0XFFFFE : value;
  

  v1 = D3Y;     // Ternary operator - if wire is available then read value of v1 else write 0
  v2 = D2Y;     // Ternary operator - if wire is available then read value of v2 else write 0
  v3 = D1Y;     // Ternary operator - if wire is available then read value of v3 else write 0
  d = (v1 << 16) | (v2 << 8) | v3;      // Bit shift v1 16 bits to the left, v2 8 bits to the left
  d = d >> 4;                                 // Bit shift data 4 bits to the right
  value = (float)d;                        // value = data
  mediciones[1] = d > 0X7FFFF ? value - 0XFFFFE : value;
  

  v1 = D3Z;     // Ternary operator - if wire is available then read value of v1 else write 0
  v2 = D2Z;     // Ternary operator - if wire is available then read value of v2 else write 0
  v3 = D1Z;     // Ternary operator - if wire is available then read value of v3 else write 0
  d = (v1 << 16) | (v2 << 8) | v3;      // Bit shift v1 16 bits to the left, v2 8 bits to the left
  d = d >> 4;                                 // Bit shift data 4 bits to the right
  value = (float)d;                        // value = data
  mediciones[2] = d > 0X7FFFF ? value - 0XFFFFE : value;

}


float read_temp() {
  uint32_t D1;
  uint32_t D2;
  Wire.beginTransmission(ADXL_address); // Begin transmission to the Sensor
  Wire.write(TEMP1);
  Wire.endTransmission();
  Wire.requestFrom(ADXL_address, 1);
  if (Wire.available()) {
    D1 = Wire.read();
  }


  Wire.beginTransmission(ADXL_address); // Begin transmission to the Sensor
  Wire.write(TEMP2);
  Wire.endTransmission();
  Wire.requestFrom(ADXL_address, 1);
  if (Wire.available()) {
    D2 = Wire.read();
  }
  uint32_t v1 = D2;     // Ternary operator - if wire is available then read value of v1 else write 0
  uint32_t v2 = D1;     // Ternary operator - if wire is available then read value of v2 else write 0
  uint32_t d = (v1 << 8) | v2;      // Bit shift v1 16 bits to the left, v2 8 bits to the left
  d = d >> 4;                                 // Bit shift data 4 bits to the right
  float value = (float)d;                        // value = data
  value = d > 0X7FF ? value - 0XFFE : value;
  return value;


}


float correccion(float lectura, float offset, float gain) {    // Definimos una funcion para ajustar los valores del offset y la ganancia de
  float acc = (lectura / gain) - offset; // los sensores, que se calculan con la calibracion.
  return acc;
}
