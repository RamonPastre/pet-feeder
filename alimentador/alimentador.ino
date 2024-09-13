#include <ESP8266WiFi.h>              // Biblioteca para Wi-Fi no ESP8266
#include <Firebase_ESP_Client.h>       // Biblioteca para comunicação com Firebase
#include <NTPClient.h>                // Biblioteca para obter a hora via NTP
#include <WiFiUdp.h>                  // Biblioteca para UDP

// Configurações das redes Wi-Fi
const char* ssidList[] = { "Ramon", "RamonPastre"};
const char* passwordList[] = {"50F958F4E554", "ramon1234"};
const int numNetworks = sizeof(ssidList) / sizeof(ssidList[0]);

// Configurações do Firebase
#define DATABASE_URL "pet-feeder-canil-default-rtdb.firebaseio.com"  // URL do seu projeto Firebase
#define API_KEY "AIzaSyCkIKGGIhbCrnO9dGJwO6vDZVmKXjKAJC0"              // Chave da API Firebase

FirebaseData firebaseData;   // Objeto para manipulação do Firebase
FirebaseAuth auth;           // Autenticação Firebase
FirebaseConfig config;       // Configurações do Firebase

String motorStatusPath = "/motorStatus";  // Caminho no Firebase para o status do motor
String logsPath = "/logs";                // Caminho para armazenar o histórico de acionamentos

// Configurações de NTP (obtenção de horário)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000);  // UTC-3 para horário de Brasília

// Pinos da ponte H L298N
const int IN1 = 5;  // GPIO 5 (D1)
const int IN2 = 4;  // GPIO 4 (D2)
const int ENA = 0;  // GPIO 0 (D3)

// Duração do acionamento do motor
const int duracaoMotorSegundos = 10;   // Duração do acionamento em segundos

void setup() {
  Serial.begin(115200);

  // Conectando ao Wi-Fi
  conectarWifi();

  // Inicializando NTPClient para obter o horário
  timeClient.begin();

  // Configurando os pinos da ponte H como saídas
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  // Inicializando a ponte H com o motor desligado
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0);  // Motor desligado

  // Configurações do Firebase
  config.database_url = DATABASE_URL;
  config.api_key = API_KEY;
  
  // Configura a autenticação do Firebase
  auth.user.email = "ramon@teste.com"; // Substitua pelo seu e-mail de autenticação do Firebase
  auth.user.password = "123456";        // Substitua pela sua senha de autenticação do Firebase

  // Inicializa Firebase com as configurações e autenticação
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Configura o callback para verificar o status do token
  config.token_status_callback = tokenStatusCallback; // Função de callback
  
  Serial.println("Conectado ao Firebase.");
}

void loop() {
  // Verifica se o Wi-Fi está conectado, se não, tenta reconectar
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi desconectado. Tentando reconectar...");
    conectarWifi();
  }

  // Atualiza o horário via NTP
  timeClient.update();

  // Verifica o valor no Firebase
  if (Firebase.RTDB.getInt(&firebaseData, motorStatusPath)) {
    int motorStatus = firebaseData.intData();
    Serial.print("Status do motor no Firebase: ");
    Serial.println(motorStatus);

    // Se o valor for 1, aciona o motor
    if (motorStatus == 1) {
      ligarMotor();

      // Após acionar o motor, redefine o valor para 0 e atualiza o status para "desligado"
      Firebase.RTDB.setInt(&firebaseData, motorStatusPath, 0);
      
      // Salva o registro do acionamento
      salvarLogAcionamento();
    }
  } else {
    Serial.println("Erro ao ler do Firebase: " + firebaseData.errorReason());
  }

  delay(2000);  // Intervalo entre as verificações
}

// Função de callback para verificar o status do token
void tokenStatusCallback(TokenInfo info) {
  if (info.status == token_status_ready) {
    Serial.println("Token está pronto.");
  } else if (info.status == token_status_error) {
    Serial.printf("Erro no token: %s\n", info.error.message.c_str());
  }
}

// Função para ligar o motor
void ligarMotor() {
  Serial.println("Motor ligado!");

   analogWrite(ENA, 255);  // Velocidade máxima
  for (int i = 0; i < 7; i++) {  // Repete o ciclo 7 vezes
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    delay(3500);  // Mantém o motor ligado por 3.5 segundos

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    delay(500);   // Alterna a direção do motor por 0.5 segundos
  }

  // Último ciclo com 2 segundos
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  delay(2000);

  // Desliga o motor após o tempo determinado
  analogWrite(ENA, 0);  // Desligar o motor
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  Serial.println("Motor desligado!");
}

// Função para salvar um log com o horário do acionamento
void salvarLogAcionamento() {
  // Obtém o horário atual via NTP
  String currentTime = getFormattedTime();
  Serial.print("Salvando log de acionamento no horário: ");
  Serial.println(currentTime);

  // Gera um ID único baseado no timestamp para o log
  String logID = String(timeClient.getEpochTime());

  // Cria um novo registro no Firebase com o horário do acionamento
  Firebase.RTDB.setString(&firebaseData, logsPath + "/" + logID, currentTime);
  Serial.println("Log de acionamento salvo.");
}

// Função para obter o horário formatado como string (HH:MM:SS DD/MM/YYYY)
String getFormattedTime() {
  time_t rawTime = timeClient.getEpochTime();
  struct tm* timeInfo = localtime(&rawTime);

  char timeString[20];
  strftime(timeString, sizeof(timeString), "%H:%M:%S %d/%m/%Y", timeInfo);
  
  return String(timeString);
}

// Função para conectar ao Wi-Fi
void conectarWifi() {
  bool connected = false;
  
  for (int i = 0; i < numNetworks; i++) {
    Serial.print("Tentando conectar a: ");
    Serial.println(ssidList[i]);

    WiFi.begin(ssidList[i], passwordList[i]);

    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED && retryCount < 10) {
      delay(1000);
      Serial.print(".");
      retryCount++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("Conectado ao WiFi");
      connected = true;
      break;
    } else {
      Serial.println("");
      Serial.println("Falha ao conectar ao Wi-Fi.");
    }
  }

  if (!connected) {
    Serial.println("Não foi possível conectar a nenhuma rede. Reiniciando...");
    ESP.restart();
  }
}
