#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHTesp.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define DHT_PIN 19
#define BUZZER_PIN 18

const char* ssid = "";
const char* password = "";

const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

DHTesp dht;

void setupWifi() {

  Serial.println();
  Serial.print("Conectando WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado!");

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {

  while (!client.connected()) {

    Serial.print("Conectando MQTT...");

    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {

      Serial.println(" conectado!");

    } else {

      Serial.print("Falhou. Codigo: ");
      Serial.println(client.state());

      delay(2000);
    }
  }
}

void setup() {

  Serial.begin(115200);

  dht.setup(DHT_PIN, DHTesp::DHT11);

  Wire.begin(21,22);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C)) {

      Serial.println("Erro OLED");

      while(true);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  pinMode(BUZZER_PIN, OUTPUT);

  setupWifi();

  client.setServer(mqtt_server,1883);

  Serial.println("Sistema iniciado");
}

void loop() {

  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  TempAndHumidity data =
      dht.getTempAndHumidity();

  float temp = data.temperature;
  float hum = data.humidity;

  if (isnan(temp) || isnan(hum)) {

    Serial.println("Erro sensor");
    return;
  }

  Serial.print("Temperatura: ");
  Serial.print(temp);
  Serial.print(" C");

  Serial.print(" | Umidade: ");
  Serial.print(hum);
  Serial.println(" %");

  display.clearDisplay();

  display.setCursor(0,10);
  display.print("Temp: ");
  display.print(temp);
  display.println(" C");

  display.setCursor(0,30);
  display.print("Umid: ");
  display.print(hum);
  display.println(" %");

  if(temp > 35){

    display.setCursor(0,50);
    display.println("!!! ALERTA !!!");
  }

  display.display();

  char tempStr[50];
  char humStr[50];

  sprintf(
      tempStr,
      "Temperatura: %.2f C",
      temp
  );

  sprintf(
      humStr,
      "Umidade: %.2f %%",
      hum
  );

  client.publish(
      "adsobj/esp32/temperatura",
      tempStr
  );

  client.publish(
      "adsobj/esp32/umidade",
      humStr
  );

  Serial.println("Dados enviados MQTT");

  if(temp > 35){

      tone(BUZZER_PIN,1000);

      delay(200);

      noTone(BUZZER_PIN);
  }

  Serial.println("----------------");

  delay(2000);
}