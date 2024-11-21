#include <HX711.h>       // Biblioteca para a célula de carga
#include <WiFi.h>        // Biblioteca para conexão WiFi
#include <PubSubClient.h> // Biblioteca MQTT

// Configuração dos pinos para o módulo HX711
const int PINO_DT = 3;
const int PINO_SCK = 2;

const int TEMPO_ESPERA = 1000; // Tempo de espera entre leituras (ms)

// Variáveis WiFi e MQTT
const char* ssid = "SEU_SSID";          // Substitua pelo seu SSID WiFi
const char* password = "SUA_SENHA";    // Substitua pela sua senha WiFi
const char* mqtt_server = "BROKER_IP"; // Substitua pelo IP ou domínio do broker MQTT
const char* mqtt_topic = "balanca/peso"; // Tópico MQTT para publicar os dados

WiFiClient espClient;
PubSubClient client(espClient);

// Objeto da célula de carga
HX711 escala;

// Fator de calibração (ajuste conforme necessário)
float fator_calibracao = -45000.0;
char comando; // Comando para ajuste do fator de calibração

void setup() {
  Serial.begin(9600);
  Serial.println("Célula de carga - Calibração de Peso com MQTT");

  // Conexão WiFi
  connectWiFi();

  // Configuração do MQTT
  client.setServer(mqtt_server, 1883);

  // Inicialização da balança
  escala.begin(PINO_DT, PINO_SCK);
  float media_leitura = escala.read_average();
  Serial.print("Média de leituras com célula sem carga: ");
  Serial.println(media_leitura);
  escala.tare(); // Zera a escala
}

void loop() {
  // Verifica conexão com o MQTT e reconecta se necessário
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  escala.set_scale(fator_calibracao); // Ajusta o fator de calibração

  if (escala.is_ready()) {
    float peso = escala.get_units() * -1; // Leitura em kg
    Serial.print("Leitura: ");
    Serial.print(peso, 1);
    Serial.print(" kg \t Fator de Calibração = ");
    Serial.println(fator_calibracao);

    // Publica os dados no MQTT
    String payload = String(peso, 1) + " kg";
    client.publish(mqtt_topic, payload.c_str());

    // Ajuste fino do fator de calibração
    if (Serial.available()) {
      comando = Serial.read();
      switch (comando) {
        case 'x': fator_calibracao -= 100; break;
        case 'c': fator_calibracao += 100; break;
        case 'v': fator_calibracao -= 10; break;
        case 'b': fator_calibracao += 10; break;
        case 'n': fator_calibracao -= 1; break;
        case 'm': fator_calibracao += 1; break;
      }
    }
  } else {
    Serial.println("HX711 ocupado");
  }

  delay(TEMPO_ESPERA);
}

void connectWiFi() {
  Serial.print("Conectando ao WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (client.connect("ArduinoClient")) {
      Serial.println("Conectado!");
    } else {
      Serial.print("Falha na conexão. Código: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}
