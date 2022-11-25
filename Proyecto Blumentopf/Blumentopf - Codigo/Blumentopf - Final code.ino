#include <DHT_U.h>
#include <DHT.h>
//Librerias para ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
DHT SENSOR_DE_TEMPERATURA (D1, DHT11);

#include "data.h"
#define HUMEDAD_DEL_SUELO A0

int Agua = 2; //Bomba de agua
int pinLed = 0; //Lampara
boolean Estado_agua = false;
boolean Estado_led = false;
boolean Estado_Timer = LOW;
int HUMEDAD, TEMPERATURA;
int LAMPARA_EN_ON = 10000;  //Regula el tiempo en ON de la lampara
int LAMPARA_EN_OFF = 10000;  //Regula el tiempo en OFF de la lampara
unsigned long TIEMPO_DE_LAMPARA = millis();
unsigned long TIEMPO_LECTURA = millis();

const uint32_t TiempoEsperaWifi = 5000;

unsigned long TiempoActual = 0;
unsigned long TiempoAnterior = 0;
const long TiempoCancelacion = 2000;

WiFiServer servidor(80);

void setup() {
  Serial.begin(9600);
  Serial.println("\nIniciando multi Wifi");

  pinMode(pinLed, OUTPUT);
  digitalWrite(pinLed, 0);

  pinMode(Agua, OUTPUT);
  digitalWrite(Agua, 0);

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
  WiFiClient cliente = servidor.available();

  if (millis()-TIEMPO_LECTURA>=3000){
  HUMEDAD = analogRead(HUMEDAD_DEL_SUELO);
  HUMEDAD = map(HUMEDAD, 200, 1023, 100, 0);
  Serial.println(HUMEDAD);
  Serial.print("Bomba de agua");
  Serial.println(Estado_agua);
  Serial.print("Lampara");
  Serial.println(Estado_led);
  TIEMPO_LECTURA = millis();
  }
  TEMPERATURA = SENSOR_DE_TEMPERATURA.readTemperature();

  if (HUMEDAD>=750 && TEMPERATURA<=40){
    digitalWrite(Agua, HIGH);
}
  if (HUMEDAD<=400){
    digitalWrite(Agua, LOW);
}


  if(Estado_Timer==LOW && millis()-TIEMPO_DE_LAMPARA>=LAMPARA_EN_OFF){
  digitalWrite (pinLed, HIGH);
  Estado_Timer = HIGH;
  TIEMPO_DE_LAMPARA=millis();
  }
  if(Estado_Timer==HIGH && millis()-TIEMPO_DE_LAMPARA>=LAMPARA_EN_ON){
    digitalWrite (pinLed, LOW);
    Estado_Timer = LOW;
    TIEMPO_DE_LAMPARA=millis();
  }


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
    Estado_led = false; digitalWrite (pinLed, LOW);
  }
  if (Mensaje.indexOf("GET /encender_bomba") >= 0) {
    Serial.println("Encender Led");
    Estado_agua = true; digitalWrite (Agua, HIGH);
  } else if (Mensaje.indexOf("GET /apagar_bomba") >= 0) {
    Serial.println("Apagar Led");
    Estado_agua = false; digitalWrite (Agua, LOW);
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
  cliente.print("<br>Estado de la bomba: ");
  cliente.print(Estado_agua ? "Encendida" : "Apagada");
  cliente.print("<br>Cambia la bomba: ");
  cliente.print("<a href = '/");
  cliente.print(Estado_agua ? "apagar_bomba" : "encender_bomba");
  cliente.print("'>Cambiar </a><br>");
  cliente.print("</html>");
}
