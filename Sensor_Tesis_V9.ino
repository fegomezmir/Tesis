
#include <Wire.h>

#include "FS.h"           // Librerias escritura en trajeta SD
#include "SD.h"
#include "SPI.h"

#include <WiFi.h>
#include "time.h"
#include "RTClib.h"

#include <FirebaseESP32.h>

//******************** Configuracion *************
const int frecuencia_muestreo = 100;// se aceptan solo multiplos de 2 o de 5;
const int tiempo_de_registro = 1 * 1000; // duracion del tiempo de registro(en segundos).
const float sensibilidad_actividad = 1.2; // cuantos g varia la medicion respecto de la media de las mediciones

#define WIFI_SSID "Galaxy FG"
#define WIFI_PASSWORD "fenderyfiona"
#define FIREBASE_HOST "tesis-4bc6a-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "Hf9nrOPEQ09LJEepeNiDtvkkFGwyWlkKkVOZemW0"

const int SENSOR_ID = 1;
FirebaseData firebaseData;

int led_azul = 15;
int led_verde = 14;
int led_amarillo = 32;

//******************** Velocidad de muestreo *************





int periodo_muestreo = 1000000 / frecuencia_muestreo; // transformacion de frecuencia a tiempo en microsegundos

int t_anterior;



// ===================================================================================================================================================================================================================================
//********************************************************************************************** Tiempo ************************************************************************************************************************
//====================================================================================================================================================================================================================================


const char* ntpServer = "south-america.pool.ntp.org";
const long  gmtOffset_sec = -5 * 3600;
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

void set_rtc_time(int ano, int mes, int dia, int hora, int minuto, int segundo) {
  rtc.adjust(DateTime(ano, mes, dia, hora, minuto, segundo));
}

void print_time() {
  DateTime now = rtc.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

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


byte value;

const int READ_BYTE = 0x01;
const int WRITE_BYTE = 0x00;



int tiempo_med;
float X;
float Y;
float Z;



int num_muestra = 1;
bool data_ready = 0;
bool actividad_detectada = 0;
bool registrando = 0;
int t_inicio_registro;



struct muestra { // paquete de datos conformado por el tiempo de medicion y lectura de los tres ejes del sensor
  int tiempo;
  float x;
  float y;
  float z;
  int ano;
  int mes;
  int dia ;
  int hora;
  int minuto;
  int segundo;

};


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

  if (escritura_empty == 1 and buffer1_empty == 0 ) {
    memcpy(&escritura, &buffer3, sizeof(escritura));
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
  if (x_anterior - x > sensibilidad_actividad) {
    actividad_detectada = 1;
  }
  if (x_anterior - x > sensibilidad_actividad) {
    actividad_detectada = 1;
  }
  if (x_anterior - x > sensibilidad_actividad) {
    actividad_detectada = 1;
  }
  else {
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
    registrando = 1;
    t_inicio_registro = micros();
  }
  if (micros() - t_inicio_registro >= tiempo_de_registro + 10000000) {
    registrando = 0;
  }
}




void muestreo() {

  if (micros() - t_anterior >= periodo_muestreo) {
    t_anterior = micros();

    X = correccion(read_acc(XDATA1, XDATA2, XDATA3), 0, 256410);
    Y = correccion(read_acc(YDATA1, YDATA2, YDATA3), 0, 256410);
    Z = correccion(read_acc(ZDATA1, ZDATA2, ZDATA3), 0, 256410);
    tiempo_med = millis();



    muestra muestra0;

    muestra0.tiempo = tiempo_med;
    muestra0.x = X;
    muestra0.y = Y;
    muestra0.z = Z;

    DateTime now = rtc.now();
    muestra0.dia = (int)(now.day());
    muestra0.mes = (int)(now.month());
    muestra0.ano = (int)(now.year());
    muestra0.hora = (int)(now.hour());
    muestra0.minuto = (int)(now.minute());
    muestra0.segundo = (int)(now.second());

    Serial.print(muestra0.x);
    Serial.print(", ");
    Serial.print(muestra0.y);
    Serial.print(", ");
    Serial.println(muestra0.z);


    deteccion_actividad(X, Y, Z, sensibilidad_actividad);
    x_anterior = X;
    y_anterior = Y;
    z_anterior = Z;

    registrar();

    if (registrando == 1) {
      digitalWrite(led_verde, HIGH);
      Serial.println("Registrando.......");
      armar_buffer(muestra0);
    }
    else{
      digitalWrite(led_verde, LOW);
      }


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
      delay(1); // esta funcion es sensible al tiempo ARREGLAR
      if (escritura_empty == 0) {
        Serial.println(escritura.muestra1.z);
        Serial.println(escritura.muestra5.z);
        Serial.println(escritura.muestra10.z);
        Serial.println(escritura.muestra15.z);
        Serial.println(escritura.muestra20.z);
        Serial.println(escritura.muestra25.z);
        Serial.println(escritura.muestra30.z);

        Serial.println("escribiendo........");
        delay(100);
        memcpy(&escritura, &set_muestra, sizeof(escritura));
        if (file.write( (const uint8_t *)&escritura, sizeof(escritura))) {
          Serial.println("File written");
          escritura_empty == 1;
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
muestra muestra_lectura;


int tiempo_lectura;
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
String string_ano = "";

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
  float valor_float;
  byte valor_muestra[4] ;
  Serial.print("Read from file: ");



  union valor_int {
    byte b[4];
    int  ival;
  } valor_lectura_int;


  union valor_float {
    byte b[4];
    float fval;
  } valor_lectura_float;


  while (file.available()) {
    if (num_valor == 0) {
      for (int pos_byte = 0; pos_byte < 4; pos_byte++) {
        valor_lectura_int.b[pos_byte] =  file.read();
      }
      Serial.print("valor leido tiempo: ");
      Serial.println(valor_lectura_int.ival);
      muestra_lectura.tiempo =  valor_lectura_int.ival;
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
      muestra_lectura.ano =  valor_lectura_int.ival;
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
      string_ano = string_ano + "," + String(muestra_lectura.ano);

      numero_lectura += 1;

      num_valor = 0;
      if (numero_lectura > 100) { // 100 mediciones por bloque, con más colapsa
        lectura_lista = 1;
      }

    }

  }
  file.close();
  deleteFile(SD, path);
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
        break;
        num_archivo = 0;
      }
      file.close();
      readFile(SD, nombre_archivo2);
      num_archivo += 1;
    }
  }
  else {
    connect_wifi();
  }
}

void send_data(String tiempo, String x, String y, String z, String ano, String mes, String dia, String hora, String minuto, String segundo) {
  digitalWrite(led_amarillo, HIGH);
  FirebaseJson medicion;

  medicion.add("tiempo", tiempo).add("X", x).add("Y", y).add("Z", z).add("Agno", ano).add("Mes", mes).add("Dia", dia).add("Hora", hora).add("Minuto", minuto).add("Segundo", segundo);


  if (Firebase.pushJSON(firebaseData, "/mediciones/pool/" + String(SENSOR_ID), medicion)) {
    Serial.println("DATA push JSON data success");
  }
  
  else {
    Serial.print("Error in push JSON, ");
    Serial.println(firebaseData.errorReason());
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
      break;
    }
  }
}



// ===================================================================================================================================================================================================================================
//********************************************************************************************** Configuracion de tareas ************************************************************************************************************************
//====================================================================================================================================================================================================================================



TaskHandle_t Task1;
TaskHandle_t Task2;

void Task1code( void * pvParameters ) {
  while (true) {
    muestreo();
    buffer_shift();
    if (lectura_lista ) {
      Serial.print("String_x:");
      Serial.println(string_x);
      send_data(string_tiempo, string_x, string_y, string_z, string_ano, string_mes, string_dia, string_hora, string_minuto, string_segundo );
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
      string_ano = "";

    }
  }
}

void Task2code( void * pvParameters ) {
  while (true) {
    check_wifi();
    if (registrando == 1) {
      escribiendo(SD);
    }
    else {
      upload(SD);
    }

  }
}

void setup() {

  pinMode(led_azul, OUTPUT);
  pinMode(led_verde, OUTPUT);
  pinMode(led_amarillo, OUTPUT);

  Serial.begin(250000);                 // Inicio terminal Serial

  connect_wifi();


  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  Wire.begin(); //initiate the accelerometer
  iniciar_adxl();
  set_range_2g();
  set_sample_rate();
  delay(100);

  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    ESP.restart();
    return;
  }

  check_rtc_connection();


  struct tm tiempo;
  tiempo = pedir_tiempo();
  set_rtc_time(tiempo.tm_year - 100, tiempo.tm_mon + 1 , tiempo.tm_mday, tiempo.tm_hour, tiempo.tm_min, tiempo.tm_sec);


  disableCore0WDT(); // desactiva el watchdog interno del nucleo 0


  xTaskCreatePinnedToCore(
    Task1code, /* Task function. */
    "Task1",   /* name of task. */
    10000,     /* Stack size of task */
    NULL,      /* parameter of the task */
    5,         /* priority of the task */
    &Task1,    /* Task handle to keep track of created task */
    0);        /* pin task to core 0 */

  xTaskCreatePinnedToCore(
    Task2code, /* Task function. */
    "Task2",   /* name of task. */
    10000,     /* Stack size of task */
    NULL,      /* parameter of the task */
    5,         /* priority of the task */
    &Task2,    /* Task handle to keep track of created task */
    1);        /* pin task to core 1 */

}


void loop() {

}
