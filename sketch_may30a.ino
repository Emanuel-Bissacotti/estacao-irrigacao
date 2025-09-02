# include <Arduino.h>
# include <WiFi.h>
# include <WiFiClientSecure.h>
# include <PubSubClient.h>
#include "DHT.h"

 // Credenciais de Wi-Fi 
const  char * ssid = "UFN" ; // Adicionar o nome da rede Wi-Fi
const  char * password = "" ; // Adicionar a senha da rede Wi-Fi

// Detalhes do broker MQTT private 
const  char * mqtt_broker = ""; // Adicionar o endereço do broker MQTT
const  int mqtt_port = 8883 ; 
const  char * mqtt_username = "" ; // Nome de usuário do broker MQTT
const  char * mqtt_password = "" ; // Senha do broker MQTT

// Tópico MQTT para sensor de infravermelho 
const  char * topic_publish_ir = "esp32/umidade" ; 

// Detalhes do sensor de infravermelho 
const int pinoDHT = 12;
const int pinoFC_28 = 32;

// Criar instâncias
WiFiClientSecure wifiClient; 
PubSubClient mqttClient (wifiClient) ; 
DHT dht(pinoDHT, DHT11);

void setupMQTT()  { 
  mqttClient.setServer(mqtt_broker, mqtt_port); 
  mqttClient.setCallback(mqtt_callback);
} 

void reconnect ()  { 
  Serial. println ( "Conectando ao MQTT Broker..." ); 
  while (!mqttClient.connected ()) { 
    Serial.println ( "Reconectando ao MQTT Broker..." ); 
    String clientId = "ESP32Client-" ; 
    clientId += String ( random ( 0xffff ), HEX); 
    
    if (mqttClient. connect (clientId. c_str (), mqtt_username, mqtt_password)) { 
      Serial.println( "Conectado ao MQTT Broker." ); 
    } else { 
      Serial.print("Falha, rc="); 
      Serial.print(mqttClient.state ()); 
      Serial.println("tente novamente em 5 segundos"); 
      delay ( 5000 ); 
    } 
  }
} 

void setup() { 
  Serial.begin(9600); 
  
  WiFi.begin(ssid, password); 
  while (WiFi. status () != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  } 
  Serial.println(""); 
  Serial.println("Conectado ao Wi-Fi"); 

  wifiClient.setInsecure();
  
  setupMQTT(); 
  dht.begin();

  pinMode(pinoDHT, INPUT);
  pinMode(pinoFC_28, INPUT);
  pinMode(pin_rele, OUTPUT);
} 

void loop ()  { 
  if (!mqttClient.connected ()) { 
    reconnect(); 
  } 
  mqttClient.loop (); 
  if (!mqttClient.subscribe("esp32/")) {
    Serial.println("Erro ao se inscrever no topico");
    Serial.println("Desconectando do servidor...");
    mqttClient.disconnect();
    Serial.println("Desconectado.");
  }
}

void mqtt_callback(char* topico, byte* payload, unsigned int comprimento) {
  String mensagem_recebida;

  for (int i = 0; i < comprimento; i++) {
    char c = (char)payload[i];
    mensagem_recebida += c;
  }

  if (mensagem_recebida == "Ler sensor") {
    float temperatura = dht.readTemperature();
    float umidade = dht.readHumidity();
    float analog_soil_humid = analogRead(pinoFC_28);

    String umidade_string = String (umidade); 
    String temperatura_string = String(temperatura);
    Serial.print("Analogico: ");
    Serial.println(analog_soil_humid);
    float percent_soil = ((4095 - analog_soil_humid) / (4095-1514)) * 100; // Na agua ele foi até 2006
    // if (percent_soil > 100){
    //   percent_soil = 100;
    // }
    Serial.println(percent_soil);
    Serial.println("Valor publicados: ");
    mqttClient.publish("esp32/umidade", String(umidade).c_str());
    mqttClient.publish("esp32/temperatura", String(temperatura).c_str());
    mqttClient.publish("esp32/umidade-solo", String(percent_soil).c_str());
  } else {
    // Separar a mensagem usando ":" como delimitador
    int separatorIndex = mensagem_recebida.indexOf(':');
    
    if (separatorIndex != -1) {
      String comando = mensagem_recebida.substring(0, separatorIndex);
      String quantidade_str = mensagem_recebida.substring(separatorIndex + 1);
      
      // Converter quantidade para inteiro
      int quantidade = quantidade_str.toInt();
      
      Serial.print("Comando: ");
      Serial.println(comando);
      Serial.print("Quantidade: ");
      Serial.println(quantidade);
      
      // Verificar se é comando de irrigação
      if (comando == "Irrigar") {
        Serial.println("Iniciando irrigação...");
        Serial.println(quantidade);
        digitalWrite(pin_rele, HIGH);
        sleep(quantidade * 1); // quantidade em segundos
        digitalWrite(pin_rele, LOW);
        Serial.println("Irrigação finalizada.");
      }
    } else {
      Serial.println("Formato de mensagem inválido. Use: comando:quantidade");
    }
  }
}