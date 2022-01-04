#include <Wire.h>

#include "FS.h"           // Librerias escritura en trajeta SD
#include "SD.h"
#include "SPI.h"

#include <WiFi.h>
#include "time.h"
#include "RTClib.h"

#define CIRCULAR_BUFFER_INT_SAFE
#include <CircularBuffer.h>



#include <FirebaseESP32.h>

//******************** Configuracion *************
const int SENSOR_ID = 8; //Nombre del sensor
const int tiempo_de_registro = 30; // duracion del tiempo de registro tras ultima activación del threshold(en segundos).
const float sensibilidad_actividad = 0.01; // cuantos g varia la media del buffer corto respecto de la media del buffer largo. Valor mínimo recomendado 0.005

#define FIREBASE_HOST "Host-del-proyecto.firebaseio.com"
#define FIREBASE_AUTH "Clave-Auth"
#define WIFI_SSID "wifi-ssid"
#define WIFI_PASSWORD "password"




//******************** Definicion de variables globales *************

const float frecuencia_muestreo = 125;// se aceptan solo multiplos de 2 o de 5;
const int largo_buffer = 300;
const int largo_filtrado = 5;

FirebaseData firebaseData;

int led_azul = 15;
int led_verde = 32;
int led_amarillo = 14;

int control_manual = 21;

struct muestra { // paquete de datos conformado por el tiempo de medicion y lectura de los tres ejes del sensor
  unsigned int tiempo;
  float x;
  float y;
  float z;
  int agno;
  int mes;
  int dia ;
  int hora;
  int minuto;
  int segundo;
};

CircularBuffer <muestra, largo_buffer > shift_register;
CircularBuffer <muestra, largo_filtrado > filtro;



//******************** Velocidad de muestreo *************

int periodo_muestreo = 1000 / frecuencia_muestreo; // transformacion de frecuencia a tiempo en milisegundos

float t_anterior;



// ===================================================================================================================================================================================================================================
//********************************************************************************************** Tiempo ************************************************************************************************************************
//====================================================================================================================================================================================================================================


const char* ntpServer = "200.27.106.115";
const long  gmtOffset_sec = -4 * 3600;
const int   daylightOffset_sec = 3600;

RTC_DS3231 rtc;

struct tm pedir_tiempo() {

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
  }
  return timeinfo ;
}


void check_rtc_connection() {
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }
}

void set_rtc_time(int agno, int mes, int dia, int hora, int minuto, int segundo) {
  rtc.adjust(DateTime(agno, mes, dia, hora, minuto, segundo));
}


bool pulso_coordinado = 0;

unsigned int tiempo = 0;
bool coordinado = 0;
int tiempo_coordinacion;
unsigned int epoch = 0;


void coordinacion() {
  Serial.print("Coordinando");


  int intentos = 0;
  int t_maximo = 0;
  int tiempo_anterior = 0;
  while (coordinado == 0) {
    if (WiFi.status() != WL_CONNECTED) {
      break;
    }
    while (intentos < 20) {

      if (millis() - tiempo_anterior >= 1000) {
        tiempo_anterior = millis();
        struct timeval tv;
        int t_roundtrip = 0;
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        int millis_inicio = millis();
        if (gettimeofday(&tv, NULL) != 0) {
          Serial.println("Failed to obtain time");
          return;
        }
        t_roundtrip = millis() - millis_inicio;
        epoch = ((unsigned int)tv.tv_sec   % 10000000) * 1000 ;
        Serial.println(epoch);
        int millis_coordinacion = (tv.tv_usec / 1000 + t_roundtrip / 2);
        if (millis_coordinacion > t_maximo) {
          t_maximo = millis_coordinacion;
        }
        Serial.println("Coordinando...");
        Serial.println(t_maximo);
        intentos += 1;
      }
      if (t_maximo < 200 || t_maximo > 800) {
        t_maximo = 0;
        intentos = 0;
        delay(200);
        tiempo_anterior = 0;
        Serial.println("corrigiendo");
      }
    }
    if (t_maximo > 200 && t_maximo < 800) {
      tiempo_coordinacion = t_maximo;
      coordinado = 1;
      Serial.println("Coordinado");
    }
    else {
      Serial.println("No Coordinado");
      t_maximo = 0;
      intentos = 0;
      delay(300);
      tiempo_anterior = 0;
    }
  }
  coordinado = 1;
  tiempo = epoch + tiempo_coordinacion;


  while (true) {
    Serial.println(tiempo);
    if (tiempo % 1000 == 520) {
      rtc.writeSqwPinMode (DS3231_SquareWave1Hz);
      struct tm tiempo_struct;
      tiempo_struct = pedir_tiempo();
      set_rtc_time(tiempo_struct.tm_year - 100, tiempo_struct.tm_mon + 1 , tiempo_struct.tm_mday, tiempo_struct.tm_hour, tiempo_struct.tm_min, tiempo_struct.tm_sec);
      break;
    }
  }
  while (pulso_coordinado == 0) {
    pulso_tiempo();
  }
  shift_register.clear();

}


bool timer_flag = 0;

void IRAM_ATTR rtc_interrupt ()
{
  timer_flag = 1;
}

float off_set = 0;

hw_timer_t * timer = timerBegin(0, 80, true);

void IRAM_ATTR sum_millis() {
  tiempo += 1;
}

bool flag_muestreo = 0;

hw_timer_t * timer_muestreo = timerBegin(1, 80, true);

void IRAM_ATTR muestrear() {
  flag_muestreo = 1;
}

const byte rtcTimerIntPin = 39;
int delta_pulso = 0;

void pulso_tiempo() {
  if (timer_flag == 1) {
    /* Serial.print("pulso: ");
      Serial.print(delta_pulso);
      Serial.print(", ");
      Serial.print(millis() % 1000);
      Serial.print(", ");
      Serial.println(tiempo % 1000);*/
    if (pulso_coordinado == 0) {
      delta_pulso = tiempo % 1000;
      pulso_coordinado = 1;
    }
    else {
      if (0) {     //(WiFi.status() == WL_CONNECTED) {
        struct timeval tv;
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        if (gettimeofday(&tv, NULL) != 0) {
          Serial.println("Failed to obtain time");
          return;
        }
        epoch = ((unsigned int)tv.tv_sec   % 10000000) * 1000 ;
        tiempo = epoch + delta_pulso;
      }
      else {
        tiempo = tiempo - (tiempo % 1000) + delta_pulso;
      }
    }
    timer_flag = 0;
  }
}

// ===================================================================================================================================================================================================================================
//********************************************************************************************** Datos sensor ************************************************************************************************************************
//====================================================================================================================================================================================================================================

const int XDATA3 = 0x08;
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


const int READ_BYTE = 0x01;
const int WRITE_BYTE = 0x00;



unsigned int tiempo_med;
float X;
float Y;
float Z;



int num_muestra = 1;
bool data_ready = 0;
bool actividad_detectada = 0;
bool registrando = 0;
int t_inicio_registro;

int millis_inicio;






struct set_muestras { // agrupaccion de mediciones que se van rellenando, una vez rellenado es copiado a un buffer para su escritura en tarjeta sd
  // Tamaño determinado por buffer de esritura tarjeta SD (512Bytes)
  muestra muestra1;
  muestra muestra2;
  muestra muestra3;
  muestra muestra4;
  muestra muestra5;
  muestra muestra6;
  muestra muestra7;
  muestra muestra8;
  muestra muestra9;
  muestra muestra10;
  muestra muestra11;
  muestra muestra12;
  muestra muestra13;
  muestra muestra14;
  muestra muestra15;
  muestra muestra16;
  muestra muestra17;
  muestra muestra18;
  muestra muestra19;
  muestra muestra20;
  muestra muestra21;
  muestra muestra22;
  muestra muestra23;
  muestra muestra24;
  muestra muestra25;
  muestra muestra26;
  muestra muestra27;
  muestra muestra28;
  muestra muestra29;
  muestra muestra30;
};

set_muestras set_muestra;
set_muestras buffer1;
set_muestras buffer2;
set_muestras buffer3;
set_muestras escritura;




bool buffer1_empty = 1;
bool buffer2_empty = 1;
bool buffer3_empty = 1;
bool escritura_empty = 1;


void buffer_shift() {

  if (escritura_empty == 1 && buffer1_empty == 0 ) {
    memcpy(&escritura, &buffer1, sizeof(escritura));
    escritura_empty = 0;
    buffer1_empty = 1;
  }
  if (data_ready == 1) {
    memcpy(&buffer1, &set_muestra, sizeof(buffer1));
    buffer1_empty = 0;
    data_ready = 0;
  }
}


float x_anterior = 1;
float y_anterior = 1;
float z_anterior = 1;

void deteccion_actividad(float x, float y, float z, float sensibilidad_actividad) {
  float media_x = 0;
  float media_y = 0;
  float media_z = 0;
  actividad_detectada = 0;

  for (int i = 0; i < shift_register.size(); i++) {
    media_x += shift_register[i].x / shift_register.size();
    media_y += shift_register[i].y / shift_register.size();
    media_z += shift_register[i].z / shift_register.size();
  }
  float media_filtro_x = 0;
  float media_filtro_y = 0;
  float media_filtro_z = 0;
  for (int i = 0; i < filtro.size(); i++) {
    media_filtro_x += filtro[i].x / filtro.size();
    media_filtro_y += filtro[i].y / filtro.size();
    media_filtro_z += filtro[i].z / filtro.size();
  }


  /*Serial.print("delta Media: ");
    Serial.print(abs(media_x - media_filtro_x), 7);
    Serial.print(", ");
    Serial.print(abs(media_y - media_filtro_y), 7);
    Serial.print(", ");
    Serial.println(abs(media_z - media_filtro_z), 7);*/
  if (abs(media_x - media_filtro_x) > sensibilidad_actividad) {
    actividad_detectada = 1;
  }
  if (abs(media_y - media_filtro_y) > sensibilidad_actividad) {
    actividad_detectada = 1;
  }
  if (abs(media_z - media_filtro_z) > sensibilidad_actividad) {
    actividad_detectada = 1;
  }
  bool estado_control_manual = digitalRead(control_manual);
  Serial.print("Estado control manual:");
  Serial.println(estado_control_manual);
  if (estado_control_manual == 1) {
    actividad_detectada = 1;
  }
  if (shift_register.size() < largo_buffer / 2) {
    actividad_detectada = 0;
  }

}

void buffer_reset() {
  set_muestra = {};
  buffer1 = {};
  buffer2 = {};
  buffer3 = {};
  escritura = {};
}

void registrar() {
  if (actividad_detectada == 1) {
    if (registrando == 0) {
      buffer_reset();
    }
    registrando = 1;
    t_inicio_registro = micros();

  }
  if (micros() - t_inicio_registro >= tiempo_de_registro * 1000000) {
    registrando = 0;
  }
}


muestra muestra0;

void muestreo() {

  if (flag_muestreo == 1) {
    flag_muestreo = 0;
    tiempo_med = tiempo;
    float acc[3];
    read_acc(acc);
    X = correccion(acc[0], 0, 256410);
    Y = correccion(acc[1], 0, 256410);
    Z = correccion(acc[2], 0, 256410);
    DateTime now = rtc.now();



    muestra0.tiempo = tiempo_med;
    muestra0.x = X;
    muestra0.y = Y;
    muestra0.z = Z;
    muestra0.dia = (int)(now.day());
    muestra0.mes = (int)(now.month());
    muestra0.agno = (int)(now.year());
    muestra0.hora = (int)(now.hour());
    muestra0.minuto = (int)(now.minute());
    muestra0.segundo = (int)(now.second());



    Serial.print(muestra0.tiempo);
    Serial.print(", ");
    Serial.print(muestra0.x);
    Serial.print(", ");
    Serial.print(muestra0.y);
    Serial.print(", ");
    Serial.print(muestra0.z);
    Serial.print(", ");
    Serial.print(muestra0.dia);
    Serial.print(", ");
    Serial.print(muestra0.mes);
    Serial.print(", ");
    Serial.print(muestra0.agno);
    Serial.print(", ");
    Serial.print(muestra0.hora);
    Serial.print(", ");
    Serial.print(muestra0.minuto);
    Serial.print(", ");
    Serial.println(muestra0.segundo);


    shift_register.unshift(muestra0);
    filtro.unshift(muestra0);

    if (shift_register.isFull()) {
      shift_register.pop();
    }
    if (filtro.isFull()) {
      filtro.pop();
    }

    deteccion_actividad(X, Y, Z, sensibilidad_actividad);
    x_anterior = X;
    y_anterior = Y;
    z_anterior = Z;

    registrar();

    if (registrando == 1) {
      digitalWrite(led_verde, HIGH);
      Serial.println("Registrando.......");
      armar_buffer(shift_register.pop());
    }
    else {
      digitalWrite(led_verde, LOW);
    }
    buffer_shift();
  }
}



void armar_buffer(struct muestra muestra_base) {
  Serial.print("numero muestra: ");
  Serial.println(num_muestra);
  if (num_muestra == 1) {
    memcpy(&set_muestra.muestra1, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 2) {
    memcpy(&set_muestra.muestra2, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 3) {
    memcpy(&set_muestra.muestra3, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 4) {
    memcpy(&set_muestra.muestra4, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 5) {
    memcpy(&set_muestra.muestra5, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 6) {
    memcpy(&set_muestra.muestra6, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 7) {
    memcpy(&set_muestra.muestra7, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 8) {
    memcpy(&set_muestra.muestra8, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 9) {
    memcpy(&set_muestra.muestra9, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 10) {
    memcpy(&set_muestra.muestra10, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 11) {
    memcpy(&set_muestra.muestra11, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 12) {
    memcpy(&set_muestra.muestra12, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 13) {
    memcpy(&set_muestra.muestra13, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 14) {
    memcpy(&set_muestra.muestra14, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 15) {
    memcpy(&set_muestra.muestra15, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 16) {
    memcpy(&set_muestra.muestra16, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 17) {
    memcpy(&set_muestra.muestra17, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 18) {
    memcpy(&set_muestra.muestra18, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 19) {
    memcpy(&set_muestra.muestra19, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 20) {
    memcpy(&set_muestra.muestra20, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 21) {
    memcpy(&set_muestra.muestra21, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 22) {
    memcpy(&set_muestra.muestra22, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 23) {
    memcpy(&set_muestra.muestra23, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 24) {
    memcpy(&set_muestra.muestra24, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 25) {
    memcpy(&set_muestra.muestra25, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 26) {
    memcpy(&set_muestra.muestra26, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 27) {
    memcpy(&set_muestra.muestra27, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 28) {
    memcpy(&set_muestra.muestra28, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 29) {
    memcpy(&set_muestra.muestra29, &muestra_base, sizeof(muestra_base));
  }
  else if (num_muestra == 30) {
    memcpy(&set_muestra.muestra30, &muestra_base, sizeof(muestra_base));
  }
  num_muestra += 1;

  if (num_muestra > 30) {
    num_muestra = 1;
    data_ready = 1;
  }

  /*Serial.print(muestra_base.x);
    Serial.print(", ");
    Serial.print(muestra_base.y);
    Serial.print(", ");
    Serial.println(muestra_base.z);*/

}



// ===================================================================================================================================================================================================================================
//********************************************************************************************** Escritura datos en trajeta SD ************************************************************************************************************************
//====================================================================================================================================================================================================================================


void writeFile(fs::FS &fs, const char * path, const char * message) {
  if (registrando == 1) {
    Serial.printf("Writing file: %s\n", path);
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
      Serial.println("Failed to open file for writing");
      //ESP.restart();
    }
    while (registrando == 1) {
      delay(1);
      if (escritura_empty == 0) {

        Serial.println("escribiendo........");
        if (file.write( (const uint8_t *)&escritura, sizeof(escritura))) {
          Serial.println("File written");
          escritura = {};
          escritura_empty = 1;

        }
        else {
          Serial.println("No escribio...");
          delay(5000);
        }
      }
    }
    file.close();
    Serial.println("Fin muestreo");
  }
}

void escribiendo(fs::FS &fs) {
  int num_archivo = 0;
  char nombre_archivo2[50];
  while (true) {
    String nombre_archivo = "/medicion" + String(num_archivo) + ".txt";

    int largo = nombre_archivo.length() + 1;

    nombre_archivo2[largo];
    nombre_archivo.toCharArray(nombre_archivo2, largo);

    File file = fs.open(nombre_archivo2);
    if (file.size() > 10)
    {
      Serial.println("Archivo " + nombre_archivo + " ya existe");
      num_archivo += 1;
    }
    else {
      break;
    }
  }

  writeFile(SD, nombre_archivo2, " ");
}


// ===================================================================================================================================================================================================================================
//********************************************************************************************** Lectura datos en trajeta SD ************************************************************************************************************************
//====================================================================================================================================================================================================================================
bool enviado = 1;
muestra muestra_lectura;


float tiempo_lectura;
float x_lectura;
float y_lectura;
float z_lectura;
int epoch_lectura;

bool lectura_lista = 0;

String string_x = "";
String string_y = "";
String string_z = "";
String string_tiempo = "";
String string_segundo = "";
String string_minuto = "";
String string_hora = "";
String string_dia = "";
String string_mes = "";
String string_agno = "";

int numero_lectura = 0;

void readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\n", path);
  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  int num_valor = 0;
  int valor_int ;
  unsigned int valor_uint;
  float valor_float;
  byte valor_muestra[4] ;
  Serial.print("Read from file: ");


  union valor_uint {
    byte b[4];
    unsigned int  uival;
  } valor_lectura_uint;

  union valor_int {
    byte b[4];
    int  ival;
  } valor_lectura_int;



  union valor_float {
    byte b[4];
    float fval;
  } valor_lectura_float;


  while (file.available()) {
    delay(1);
    if (WiFi.status() != WL_CONNECTED) {
      break;
    }

    if (enviado == 0) {
      continue;
    }

    if (num_valor == 0) {
      for (int pos_byte = 0; pos_byte < 4; pos_byte++) {
        valor_lectura_uint.b[pos_byte] =  file.read();
      }
      Serial.print("valor leido tiempo: ");
      Serial.println(valor_lectura_uint.uival);
      muestra_lectura.tiempo =  valor_lectura_uint.uival;
    }

    if (num_valor == 1) {
      for (int pos_byte = 0; pos_byte < 4; pos_byte++) {
        valor_lectura_float.b[pos_byte] =  file.read();
      }
      Serial.print("valor leido X: ");
      Serial.println(valor_lectura_float.fval);
      muestra_lectura.x = valor_lectura_float.fval;
    }
    if (num_valor == 2) {
      for (int pos_byte = 0; pos_byte < 4; pos_byte++) {
        valor_lectura_float.b[pos_byte] =  file.read();
      }
      Serial.print("valor leido Y: ");
      Serial.println(valor_lectura_float.fval);
      muestra_lectura.y = valor_lectura_float.fval;
    }
    if (num_valor == 3) {
      for (int pos_byte = 0; pos_byte < 4; pos_byte++) {
        valor_lectura_float.b[pos_byte] =  file.read();
      }
      Serial.print("valor leido Z: ");
      Serial.println(valor_lectura_float.fval);
      muestra_lectura.z = valor_lectura_float.fval;
    }
    if (num_valor == 4) {
      for (int pos_byte = 0; pos_byte < 4; pos_byte++) {
        valor_lectura_int.b[pos_byte] =  file.read();
      }
      Serial.print("valor leido Agno: ");
      Serial.println(valor_lectura_int.ival);
      muestra_lectura.agno =  valor_lectura_int.ival;
    }
    if (num_valor == 5) {
      for (int pos_byte = 0; pos_byte < 4; pos_byte++) {
        valor_lectura_int.b[pos_byte] =  file.read();
      }
      Serial.print("valor leido Mes: ");
      Serial.println(valor_lectura_int.ival);
      muestra_lectura.mes =  valor_lectura_int.ival;
    }
    if (num_valor == 6) {
      for (int pos_byte = 0; pos_byte < 4; pos_byte++) {
        valor_lectura_int.b[pos_byte] =  file.read();
      }
      Serial.print("valor leido Dia: ");
      Serial.println(valor_lectura_int.ival);
      muestra_lectura.dia =  valor_lectura_int.ival;
    }
    if (num_valor == 7) {
      for (int pos_byte = 0; pos_byte < 4; pos_byte++) {
        valor_lectura_int.b[pos_byte] =  file.read();
      }
      Serial.print("valor leido Hora: ");
      Serial.println(valor_lectura_int.ival);
      muestra_lectura.hora =  valor_lectura_int.ival;
    }
    if (num_valor == 8) {
      for (int pos_byte = 0; pos_byte < 4; pos_byte++) {
        valor_lectura_int.b[pos_byte] =  file.read();
      }
      Serial.print("valor leido Minuto: ");
      Serial.println(valor_lectura_int.ival);
      muestra_lectura.minuto =  valor_lectura_int.ival;
    }
    if (num_valor == 9) {
      for (int pos_byte = 0; pos_byte < 4; pos_byte++) {
        valor_lectura_int.b[pos_byte] =  file.read();
      }
      Serial.print("valor leido Segundo: ");
      Serial.println(valor_lectura_int.ival);
      muestra_lectura.segundo =  valor_lectura_int.ival;
    }
    num_valor += 1;

    if (num_valor > 9 & !lectura_lista) {

      string_x = string_x + "," + String(muestra_lectura.x, 15);
      string_y = string_y + "," + String(muestra_lectura.y, 15);
      string_z = string_z + "," + String(muestra_lectura.z, 15);
      string_tiempo = string_tiempo + "," + String(muestra_lectura.tiempo);
      string_segundo = string_segundo + "," + String(muestra_lectura.segundo);
      string_minuto = string_minuto + "," + String(muestra_lectura.minuto);
      string_hora = string_hora + "," + String(muestra_lectura.hora);
      string_dia = string_dia + "," + String(muestra_lectura.dia);
      string_mes = string_mes + "," + String(muestra_lectura.mes);
      string_agno = string_agno + "," + String(muestra_lectura.agno);

      numero_lectura += 1;

      num_valor = 0;
      if (numero_lectura > 50) { // 100 mediciones por bloque, con más colapsa
        lectura_lista = 1;
        enviado = 0;
      }

    }

  }
  file.close();
  if (WiFi.status() == WL_CONNECTED) {
    deleteFile(SD, path);
  }
  string_x = "";
  string_y = "";
  string_z = "";
  string_tiempo = "";
  string_segundo = "";
  string_minuto = "";
  string_hora = "";
  string_dia = "";
  string_mes = "";
  string_agno = "";
}

void deleteFile(fs::FS &fs, const char * path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}



// ===================================================================================================================================================================================================================================
//********************************************************************************************** Upload firebase ************************************************************************************************************************
//====================================================================================================================================================================================================================================



void upload(fs::FS &fs) {
  if (WiFi.status() == WL_CONNECTED) {
    int num_archivo = 0;
    char nombre_archivo2[50];
    while (true) {
      String nombre_archivo = "/medicion" + String(num_archivo) + ".txt";

      int largo = nombre_archivo.length() + 1;

      nombre_archivo2[largo];
      nombre_archivo.toCharArray(nombre_archivo2, largo);

      File file = fs.open(nombre_archivo2);
      if (file.size() > 10)
      {
        Serial.println("Archivo " + nombre_archivo + "existe");

      }
      else {
        file.close();
        num_archivo = 0;
        break;
      }
      file.close();
      readFile(SD, nombre_archivo2);
      coordinado = 0;
      num_archivo += 1;
    }
  }
  else {
    connect_wifi();
  }
}

void send_data(String tiempo, String x, String y, String z, String agno, String mes, String dia, String hora, String minuto, String segundo) {
  digitalWrite(led_amarillo, HIGH);
  FirebaseJson medicion;

  medicion.add("Millis", tiempo).add("X", x).add("Y", y).add("Z", z).add("Agno", agno).add("Mes", mes).add("Dia", dia).add("Hora", hora).add("Minuto", minuto).add("Segundo", segundo);


  if (Firebase.pushJSON(firebaseData, "/mediciones/pool/" + String(SENSOR_ID), medicion)) {
    Serial.println("DATA push JSON data success");
    lectura_lista = 0;
    numero_lectura = 0;
    string_x = "";
    string_y = "";
    string_z = "";
    string_tiempo = "";
    string_segundo = "";
    string_minuto = "";
    string_hora = "";
    string_dia = "";
    string_mes = "";
    string_agno = "";
    enviado = 1;
  }

  else {
    Serial.print("Error in push JSON, ");
    Serial.println(firebaseData.errorReason());
    delay(10000);
    digitalWrite(led_amarillo, HIGH);
    delay(50);
    digitalWrite(led_amarillo, LOW);
    delay(50);
    digitalWrite(led_amarillo, HIGH);
    delay(50);
    digitalWrite(led_amarillo, LOW);
    delay(50);
    digitalWrite(led_amarillo, HIGH);
    delay(50);
    digitalWrite(led_amarillo, LOW);
    delay(50);
    digitalWrite(led_amarillo, HIGH);
    delay(50);
    digitalWrite(led_amarillo, LOW);


  }


  medicion.iteratorEnd();
  digitalWrite(led_amarillo, LOW);

}


void check_wifi() {
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(led_azul, HIGH);
  }
  else {
    digitalWrite(led_azul, LOW);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(100);
  }
}

void connect_wifi() {
  Serial.printf("Connecting to %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int t_inicio_conexion = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - t_inicio_conexion > 10000) {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" CONNECTED");
      }
      else {
        Serial.println("NOT CONNECTED");
      }
      delay(1000);
      break;
    }
  }
}


void status_report(String text, int agno, int mes, int dia, int hora, int minuto) {
  Serial.println("Reportando estado....");
  float bateria = 2 * analogRead(A13) * 3.3 * 1.1 / 4098 ;
  Firebase.setString(firebaseData, "Estado_sensores/" + String(SENSOR_ID) + "/Estado:", text);
  Firebase.setString(firebaseData, "Estado_sensores/" + String(SENSOR_ID) + "/Fecha: ", String(agno) + "/" + String(mes) + "/" + String(dia) + " " + String(hora) + ":" + String(minuto));
  Firebase.setString(firebaseData, "Estado_sensores/" + String(SENSOR_ID) + "/Bateria: ", String(bateria) + " V" );
}


// ===================================================================================================================================================================================================================================
//********************************************************************************************** Configuracion de tareas ************************************************************************************************************************
//====================================================================================================================================================================================================================================


int error = 0;
int t_wifi = 0;
String error_text = "";
void chequeo(struct muestra muestra0) {
  error = 0;
  error_text = "OK";
  if ( muestra0.x > 3 || muestra0.x < -3) {
    error = 1; //error en acelerometro
    error_text = "Accelerometro fuera de rango";
  }
  else if ( muestra0.y > 3 || muestra0.y < -3) {
    error = 1; //error en acelerometro
    error_text = "Accelerometro fuera de rango";
  }
  else if ( muestra0.z > 3 || muestra0.z < -3) {
    error = 1; //error en acelerometro
    error_text = "Accelerometro fuera de rango";
  }
  else if (muestra0.dia > 31) {
    error = 2; //error en RTC
    error_text = "RTC fuera de rango";
  }
  else if (muestra0.mes > 12) {
    error = 2;//error en RTC
    error_text = "RTC fuera de rango";
  }
  else if (muestra0.hora > 24) {
    error = 2;//error en RTC
    error_text = "RTC fuera de rango";
  }
  else if (muestra0.agno > 2021) {
    error = 2;//error en RTC
    error_text = "RTC fuera de rango";
  }
  else if (WiFi.status() != WL_CONNECTED) {
    error = 3; //error en wifi
    error_text = "Wifi no conectado";
  }
  if (error == 1) {
    while (true) {
      digitalWrite(led_verde, HIGH);
      delay(100);
      digitalWrite(led_verde, LOW);
      delay(100);
      Serial.println(error_text);
    }
  }
  if (error == 2) {
    for (int i = 0; i < 10; i++) {
      digitalWrite(led_amarillo, HIGH);
      delay(100);
      digitalWrite(led_amarillo, LOW);
      delay(100);
      Serial.println(error_text);
    }
    struct tm tiempo_struct;
    tiempo_struct = pedir_tiempo();
    set_rtc_time(tiempo_struct.tm_year - 100, tiempo_struct.tm_mon + 1 , tiempo_struct.tm_mday, tiempo_struct.tm_hour, tiempo_struct.tm_min, tiempo_struct.tm_sec);
  }
  if (error == 3) {
    if (millis() >= t_wifi + 100) {
      t_wifi = millis();
      digitalWrite(led_azul, LOW);
    }
    Serial.println(error_text);
  }

}



TaskHandle_t Task1;
TaskHandle_t Task2;

void Task1code( void * pvParameters ) {
  while (true) {
    //Serial.print("Tiempo: ");
    //Serial.println(tiempo % 1000);

    if (registrando == 0) {
      pulso_tiempo();
      if (coordinado == 0) {
        coordinacion();
      }
    }
    muestreo();
    chequeo(muestra0);
    if (lectura_lista ) {
      send_data(string_tiempo, string_x, string_y, string_z, string_agno, string_mes, string_dia, string_hora, string_minuto, string_segundo );
    }

  }
}
int millis_report = 0;
void Task2code( void * pvParameters ) {
  while (true) {
    check_wifi();
    if (registrando == 1) {
      escribiendo(SD);
    }
    else {
      upload(SD);
    }
    if (millis() - millis_report > 60000) {
      millis_report = millis();
      status_report(error_text, muestra0.agno, muestra0.mes, muestra0.dia, muestra0.hora, muestra0.minuto);
    }
    if (tiempo % 600000 == 0) {
      coordinado = 0;
    }
  }
}

void setup() {

  pinMode(led_azul, OUTPUT);
  pinMode(led_verde, OUTPUT);
  pinMode(led_amarillo, OUTPUT);
  pinMode(control_manual, INPUT);

  pinMode (rtcTimerIntPin, INPUT_PULLUP);
  attachInterrupt (digitalPinToInterrupt (rtcTimerIntPin), rtc_interrupt, HIGH);
  timerAttachInterrupt(timer, &sum_millis, true);
  timerAlarmWrite(timer, 1000, true);
  timerAlarmEnable(timer);
  timerAttachInterrupt(timer_muestreo, &muestrear, true);
  timerAlarmWrite(timer_muestreo, periodo_muestreo * 1000, true);
  timerAlarmEnable(timer_muestreo);
  Serial.begin(250000);


  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  Wire.begin(); //initiate the accelerometer
  iniciar_adxl();
  set_range_2g();
  set_sample_rate();
  delay(100);



  check_rtc_connection();
  connect_wifi();
  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    digitalWrite(led_amarillo, HIGH);
    digitalWrite(led_verde, HIGH);
    digitalWrite(led_azul, HIGH);
    delay(100);
    status_report("Error en tarjeta SD", 0, 0, 0, 0, 0);
    delay(1000);
    ESP.restart();
    return;
  }

  struct tm tiempo_struct;
  tiempo_struct = pedir_tiempo();
  set_rtc_time(tiempo_struct.tm_year - 100, tiempo_struct.tm_mon + 1 , tiempo_struct.tm_mday, tiempo_struct.tm_hour, tiempo_struct.tm_min, tiempo_struct.tm_sec);
  rtc.writeSqwPinMode (DS3231_SquareWave1Hz);

  pedir_tiempo();
  coordinacion();
  digitalWrite(led_amarillo, LOW);
  digitalWrite(led_verde, LOW);
  digitalWrite(led_azul, LOW);


  disableCore0WDT(); // desactiva el watchdog interno del nucleo 0


  xTaskCreatePinnedToCore(
    Task1code, /* Task function. */
    "Task1",   /* name of task. */
    20000,     /* Stack size of task */
    NULL,      /* parameter of the task */
    5,         /* priority of the task */
    &Task1,    /* Task handle to keep track of created task */
    0);        /* pin task to core 0 */

  xTaskCreatePinnedToCore(
    Task2code, /* Task function. */
    "Task2",   /* name of task. */
    20000,     /* Stack size of task */
    NULL,      /* parameter of the task */
    5,         /* priority of the task */
    &Task2,    /* Task handle to keep track of created task */
    1);        /* pin task to core 1 */

}


void loop() {

}
