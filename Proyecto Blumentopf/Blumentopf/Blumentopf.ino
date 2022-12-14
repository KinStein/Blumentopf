//Librerias para el Sensor de temperatura
#include <DHT_U.h>  
#include <DHT.h>
//Librerias para ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

ESP8266WiFiMulti wifiMulti; //Declaracion del wifi
DHT SENSOR_DE_TEMPERATURA (13, DHT11); //Declaracion del sensor de temperatura

#include "data.h" //Usa la carpeta data.h
#define HUMEDAD_DEL_SUELO A0 //Declara el pin del YL69
#define Agua  D2 //Declara el pin que controla la bomba de agua
#define pinLed  D1 //Declara el pin que controla la lampara

//Variables de estado
boolean Estado_agua = false;
boolean Estado_led = false;
boolean Estado_Timer = LOW;

int HUMEDAD, TEMPERATURA;
int LAMPARA_EN_ON = 100000;  //Regula el tiempo en ON de la lampara
int LAMPARA_EN_OFF = 100000;  //Regula el tiempo en OFF de la lampara

//Variables para el uso de un timer en millis
unsigned long TIEMPO_DE_LAMPARA = millis();
unsigned long TIEMPO_LECTURA = millis();
const uint32_t TiempoEsperaWifi = 5000;

//Variables para la antencion al cliente del server Wifi
unsigned long TiempoActual = 0;
unsigned long TiempoAnterior = 0;
const long TiempoCancelacion = 2000;

WiFiServer servidor(80);

void setup() {
  Serial.begin(9600); //Iniciar comunicacion serial
  SENSOR_DE_TEMPERATURA.begin(); //Inciar el dht11
  Serial.println("\nIniciando multi Wifi");

  pinMode(pinLed, OUTPUT); //Declara como salida el pinLed
  digitalWrite(pinLed, 1); //Declara el estado del pinLed

  pinMode(Agua, OUTPUT); //Declara como salida el pin Agua
  digitalWrite(Agua, 1); //Declada el estado del pin Agua

  //Datos de las redes wifi
  wifiMulti.addAP(ssid_1, password_1); 
  wifiMulti.addAP(ssid_2, password_2);

  WiFi.mode(WIFI_STA);
  Serial.print("Conectando a Wifi ..");
  while (wifiMulti.run(TiempoEsperaWifi) != WL_CONNECTED) {
    Serial.print(".");
  }
  Serial.println(".. Conectado");
  Serial.print("SSID:");
  Serial.print(WiFi.SSID());
  Serial.print(" ID:");
  Serial.println(WiFi.localIP());

  servidor.begin();

}

void loop() {
  WiFiClient cliente = servidor.available(); //Pone disponible el server

  if (millis()-TIEMPO_LECTURA>=3000){
  HUMEDAD = analogRead(HUMEDAD_DEL_SUELO); //Lee el Yl69
  HUMEDAD = map(HUMEDAD, 200, 1024, 100, 0); //Hace una proporcional de la humedad
  TEMPERATURA = SENSOR_DE_TEMPERATURA.readTemperature(); //Lee el dht11
  TIEMPO_LECTURA = millis();
  }
  //Control automatico de la bomba de agua
  if (HUMEDAD>=50 && TEMPERATURA<=35){
    digitalWrite(Agua, 0);
}
  if (HUMEDAD<=20){
    digitalWrite(Agua, 1);
}


  if(Estado_Timer==LOW && millis()-TIEMPO_DE_LAMPARA > LAMPARA_EN_OFF){
  digitalWrite (pinLed, 0);
  Estado_Timer = HIGH;
  TIEMPO_DE_LAMPARA=millis();
  }
  if(Estado_Timer==HIGH && millis()-TIEMPO_DE_LAMPARA > LAMPARA_EN_ON){
    digitalWrite (pinLed, 1);
    Estado_Timer = LOW;
    TIEMPO_DE_LAMPARA=millis();
  }
  //Este if controla toda la conexion que existe entre el server y el cliente
  if (cliente) {
    Serial.println("Nuevo Cliente");
    TiempoActual = millis();
    TiempoAnterior = TiempoActual;
    String LineaActual = "";
    
    while (cliente.connected() && TiempoActual - TiempoAnterior <= TiempoCancelacion) {
      if (cliente.available()) {
        TiempoActual = millis();
        char Letra = cliente.read();
        if (Letra == '\n') {
          if (LineaActual.length() == 0) {
            digitalWrite(pinLed, Estado_led);
            ResponderCliente(cliente);
            break;
          } else {
            //Esto lee los mensajes del servers
            Serial.println(LineaActual);
            VerificarMensaje(LineaActual);
            LineaActual = "";
          }
        }  else if (Letra != '\r') {
          LineaActual += Letra;
        }
      }
    }

    cliente.stop();
    Serial.println("Cliente Desconectado");
    Serial.println();
  }
}

void VerificarMensaje(String Mensaje) {
  if (Mensaje.indexOf("GET /encender_led") >= 0) {
    Serial.println("Encender Led");
    Estado_led = true; digitalWrite (pinLed, HIGH); TIEMPO_DE_LAMPARA=millis();
  } else if (Mensaje.indexOf("GET /apagar_led") >= 0) {
    Serial.println("Apagar Led"); TIEMPO_DE_LAMPARA=millis();
    Estado_led = false; digitalWrite (pinLed, LOW); TIEMPO_DE_LAMPARA=millis();
  }
  }

void ResponderCliente(WiFiClient& cliente) {
  cliente.print(Pagina);
  cliente.print("Hola ");
  cliente.print(cliente.remoteIP());
  cliente.print("<br>Humedad");
  cliente.print(HUMEDAD);
  cliente.print("<br>Temperatura ");
  cliente.print(TEMPERATURA);
  cliente.print("<br>Estado del led: ");
  cliente.print(Estado_led ? "Encendida" : "Apagada");
  cliente.print("<br>Cambia el Led: ");
  cliente.print("<a href = '/");
  cliente.print(Estado_led ? "apagar_led" : "encender_led");
  cliente.print("'>Cambiar </a><br>");
  cliente.print("</html>");
}
