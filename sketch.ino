// -----------------------------------------------------------------------------
// Projeto: PoC Player Heart Rate - FIWARE + ESP32 + Potenciômetro (Simulação Wokwi)
// Autores:
//   (1ESPF)
//   Pedro Alves Faleiros - 562523
//   Luan Felix - 565541
//   João Lopes - 565737
//   Leandro Farias - 566488
//   (1ESPG)
//   Felipe Campos - 562752
// -----------------------------------------------------------------------------
// Descrição: 
// Este código conecta o ESP32 a uma rede Wi-Fi, estabelece comunicação com um 
// broker MQTT (integrado ao FIWARE), lê valores simulados de batimentos cardíacos 
// (via potenciômetro no pino 34) e envia esses dados para a nuvem. 
// Além disso, recebe comandos via MQTT para ligar/desligar o LED onboard.
// -----------------------------------------------------------------------------

#include <WiFi.h>
#include <PubSubClient.h>

// --------------------------- CONFIGURAÇÕES -----------------------------------
// Rede Wi-Fi
const char* default_SSID = "Wokwi-GUEST"; // Nome da rede Wi-Fi
const char* default_PASSWORD = "";        // Senha da rede Wi-Fi

// Broker MQTT (IP do FIWARE no Azure)
const char* default_BROKER_MQTT = "20.164.0.231"; // IP do Broker MQTT
const int default_BROKER_PORT = 1883;             // Porta padrão MQTT

// Tópicos MQTT
const char* default_TOPICO_SUBSCRIBE = "/TEF/player001/cmd";       // Para ouvir comandos (ex.: ligar/desligar LED)
const char* default_TOPICO_PUBLISH_1 = "/TEF/player001/attrs";     // Para enviar estado do LED
const char* default_TOPICO_PUBLISH_HR = "/TEF/player001/attrs/hr"; // Para enviar BPM
const char* default_TOPICO_PUBLISH_STATUS = "/TEF/player001/attrs/status"; // Para enviar status

// Identificação
const char* default_ID_MQTT = "fiware_player_001"; // ID do cliente MQTT
const int default_D4 = 2;                          // LED onboard do ESP32 (GPIO 2)

// Prefixo usado na formatação das mensagens de comando
const char* topicPrefix = "player001";

// ---------------------- VARIÁVEIS EDITÁVEIS EM TEMPO DE EXECUÇÃO -------------
char* SSID = const_cast<char*>(default_SSID);
char* PASSWORD = const_cast<char*>(default_PASSWORD);
char* BROKER_MQTT = const_cast<char*>(default_BROKER_MQTT);
int BROKER_PORT = default_BROKER_PORT;
char* TOPICO_SUBSCRIBE = const_cast<char*>(default_TOPICO_SUBSCRIBE);
char* TOPICO_PUBLISH_1 = const_cast<char*>(default_TOPICO_PUBLISH_1);
char* TOPICO_PUBLISH_HR = const_cast<char*>(default_TOPICO_PUBLISH_HR);
char* TOPICO_PUBLISH_STATUS = const_cast<char*>(default_TOPICO_PUBLISH_STATUS);
char* ID_MQTT = const_cast<char*>(default_ID_MQTT);
int D4 = default_D4;

// Objetos do Wi-Fi e MQTT
WiFiClient espClient;
PubSubClient MQTT(espClient);

// Estado atual do LED
char EstadoSaida = '0';

// --------------------------- FUNÇÕES AUXILIARES ------------------------------
void initSerial() {
    Serial.begin(115200); // Inicia a comunicação serial para debug
}

void initWiFi() {
    delay(10);
    Serial.println("------ Conexao Wi-Fi ------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    reconectWiFi(); // Conecta ao Wi-Fi
}

void initMQTT() {
    MQTT.setServer(BROKER_MQTT, BROKER_PORT); // Define servidor e porta
    MQTT.setCallback(mqtt_callback);          // Define função callback para receber mensagens
}

// --------------------------- SETUP -------------------------------------------
void setup() {
    InitOutput();   // Inicializa LED onboard piscando
    initSerial();   // Inicializa serial
    initWiFi();     // Conecta ao Wi-Fi
    initMQTT();     // Conecta ao broker MQTT
    delay(5000);

    // Publica mensagem inicial (LED ligado como status de vida do device)
    MQTT.publish(TOPICO_PUBLISH_1, "s|on");
}

// --------------------------- LOOP PRINCIPAL ----------------------------------
void loop() {
    VerificaConexoesWiFIEMQTT(); // Garante conexões ativas
    EnviaEstadoOutputMQTT();     // Envia estado atual do LED
    handleHeartRate();           // Lê e envia dados de batimentos
    MQTT.loop();                 // Mantém cliente MQTT rodando
}

// --------------------------- GERENCIAMENTO WI-FI -----------------------------
void reconectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;

    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
    Serial.println("\nConectado ao Wi-Fi!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    // Garante que o LED onboard inicie desligado
    digitalWrite(D4, LOW);
}

// --------------------------- CALLBACK MQTT -----------------------------------
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (int i = 0; i < length; i++) {
        msg += (char)payload[i]; // Monta string recebida
    }

    Serial.print("- Mensagem recebida: ");
    Serial.println(msg);

    // Formata padrões esperados para ligar/desligar LED
    String onTopic = String(topicPrefix) + "@on|";
    String offTopic = String(topicPrefix) + "@off|";

    // Se for comando de ligar
    if (msg.equals(onTopic)) {
        digitalWrite(D4, HIGH);
        EstadoSaida = '1';
    }

    // Se for comando de desligar
    if (msg.equals(offTopic)) {
        digitalWrite(D4, LOW);
        EstadoSaida = '0';
    }
}

// --------------------------- VERIFICA CONEXÕES -------------------------------
void VerificaConexoesWiFIEMQTT() {
    if (!MQTT.connected()) reconnectMQTT();
    reconectWiFi();
}

// --------------------------- ENVIO DE ESTADO ---------------------------------
void EnviaEstadoOutputMQTT() {
    if (EstadoSaida == '1') {
        MQTT.publish(TOPICO_PUBLISH_1, "s|on");
    } else {
        MQTT.publish(TOPICO_PUBLISH_1, "s|off");
    }
    delay(1000);
}

// --------------------------- INICIALIZA LED ----------------------------------
void InitOutput() {
    pinMode(D4, OUTPUT);

    // Pisca 10 vezes no início para indicar boot
    boolean toggle = false;
    for (int i = 0; i <= 10; i++) {
        toggle = !toggle;
        digitalWrite(D4, toggle);
        delay(200);
    }
    digitalWrite(D4, LOW); // Garante que termine desligado
}

// --------------------------- RECONEXÃO MQTT ----------------------------------
void reconnectMQTT() {
    while (!MQTT.connected()) {
        Serial.print("* Tentando conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);

        if (MQTT.connect(ID_MQTT)) {
            Serial.println("Conectado ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE); // Inscreve no tópico de comandos
        } else {
            Serial.println("Falha ao conectar. Nova tentativa em 2s...");
            delay(2000);
        }
    }
}

// --------------------------- LEITURA HEART RATE ------------------------------
void handleHeartRate() {
    const int potPin = 34; // Pino analógico do potenciômetro
    int sensorValue = analogRead(potPin); 
    int bpm = map(sensorValue, 0, 4095, 40, 200); // Converte valor para escala 40-200 BPM

    // Define status baseado no BPM
    String status;
    if (bpm < 60) status = "Repouso";
    else if (bpm <= 100) status = "Normal";
    else if (bpm <= 140) status = "Aquecimento";
    else status = "Alto Esforco";

    // Envia BPM e status ao broker
    String bpmMsg = String(bpm);
    MQTT.publish(TOPICO_PUBLISH_HR, bpmMsg.c_str());
    MQTT.publish(TOPICO_PUBLISH_STATUS, status.c_str());

    // Debug no Serial Monitor
    Serial.print("Heart Rate: ");
    Serial.print(bpm);
    Serial.print(" BPM | Status: ");
    Serial.println(status);
}
