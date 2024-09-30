#include <WiFi.h>
#include <HTTPClient.h>
#include <NetworkClient.h>
#include <WiFiAP.h>
#include <HardwareSerial.h>

HardwareSerial MySerial(0);

#define RxPin 3
#define TxPin 1
#define BAUDRATE 9600
#define SER_BUF_SIZE 1024
#define MODO_AP false
#define MODO_STA true
bool WifiMode{};

#define DIGITAL_VAR false
#define ANALOG_VAR true
bool dataType{};

bool UARTMode = false;

String serverIP = "";
String port = "";
String dataName = "";
int8_t dataPin{};
String pinString = "";


const char *ssid = "esp32ap";
const char *password = "microesp32";

class JSON {
public:
  String key = {};
  String value = {};
  String stringify() {
    return String("{\"" + key + "\": \"" + value + "\"}");
  }
};

IPAddress espIP;
NetworkServer server(80);
HTTPClient http;

void setup() {
  Serial.begin(9600);
  MySerial.setRxBufferSize(SER_BUF_SIZE);
  MySerial.begin(BAUDRATE, SERIAL_8N1, RxPin, TxPin);
  setupAPMode();
  server.begin();
  Serial.println("Servidor iniciado");
}

void loop() {
  NetworkClient client = server.accept();  // listen for incoming clients

  if (client) {                           // if you get a client,
    Serial.println("Cliente conectado");  // print a message out the serial port
    String currentLine = "";              // make a String to hold incoming data from the client
    bool connectFlag = false;
    bool disconnectFlag = false;
    bool modeFlag = false;
    bool parameterReading = false;
    bool sendingData = false;
    String connectParams = "?";
    String targetSSID = "";
    String targetPass = "";

    while (client.connected()) {  // loop while the client's connected
      if (client.available()) {   // if there's bytes to read from the client,
        char c = client.read();   // read a byte, then
        Serial.write(c);          // print it out the serial monitor
        if (currentLine == "GET /connect?") {
          connectFlag = true;
          parameterReading = true;
        } else if (currentLine == "GET /disconnect" && WifiMode == MODO_STA) {
          disconnectFlag = true;
        } else if (currentLine == "GET /mode") {
          modeFlag = true;
        }
        if (parameterReading) {
          if (c == ' ') {
            parameterReading = false;
          } else {
            connectParams += c;
          }
        }
        if (c == '\n') {  // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            if (modeFlag) {
              modeFlag = false;
              client.println("Content-type:application/json");
              client.println();  // El header siempre termina con una linea en blanco
              client.print("{\"mode\":\"");
              client.print(WifiMode);
              client.println("\"}");
              Serial.print("{\"mode\":\"");
              Serial.print(WifiMode);
              Serial.println("\"}");
            } else {
              client.println("Content-type:text/html");
              client.println();  // El header siempre termina con una linea en blanco
              printCfgHTML(client);
            }
            // The HTTP response ends with another blank line:
            client.println();
            if (disconnectFlag) {
              disconnectFlag = false;
              setupAPMode();
            }
            // break out of the while loop:
            break;
          } else {  // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    if (connectFlag) {
      if (connectFlag) {
        targetSSID = queryParse(connectParams, "ssid");
        targetPass = queryParse(connectParams, "password");
        serverIP = queryParse(connectParams, "server");
        port = queryParse(connectParams, "port");
        dataName = queryParse(connectParams, "name");
        pinString = queryParse(connectParams, "pin");
        dataType = setupDataPin(pinString, queryParse(connectParams, "type"));
        setupStationMode(targetSSID, targetPass);
        connectParams = "?";
        connectFlag = false;
      }
    }

    // close the connection:
    client.stop();
    Serial.println("Cliente desconectado");
    Serial.println();
  }

  // Si esta en modo station (es decir, conectado a una SSID y con un servidor de destino)
  // manda la lectura con una peticion post
  if (WifiMode == MODO_STA) {
    Serial.println("mandando data!");
    Serial.println(String("http://" + serverIP + ":" + port + "/update"));
    http.begin(String("http://" + serverIP + ":" + port + "/update"));
    http.addHeader("Content-Type", "application/json");
    JSON lectura{};
    lectura.key = dataName;
    if (UARTMode) {
      String readString = "";
      bool readFlag = false;
      while (MySerial.available()) {
        char readByte = MySerial.read();
        if (readByte == '}' && readFlag) {
            readFlag = false;
            break;
        }
        if (readFlag) {
            readString = String(readString + readByte);
        }
        if (readByte == '{') {
            readFlag = true;
        }
      }
      lectura.value = String(readString);
      Serial.println(readString);
    } else if (dataType == ANALOG_VAR) {
      lectura.value = String(analogRead(dataPin), DEC);
      Serial.println(analogRead(dataPin));
    } else {
      if (digitalRead(dataPin)) {
        lectura.value = String("true");
      } else {
        lectura.value = String("false");
      }
    }
    Serial.println(lectura.stringify());
    http.POST(lectura.stringify());
  }
  delay(1000);
}



// Funcion que manda al cliente las lineas del HTML de la web
// (escribir el codigo en VSC, hacer Join Lines y pasarlo, pero hay que generar otro print donde haya variables del ESP32)
// Las comillas dobles hay que bypassearlas con '\' (buscar y reemplazar en seleccion)
void printCfgHTML(NetworkClient client) {
  client.print("<!DOCTYPE html> <html lang=\"en\"> <head> <meta charset=\"UTF-8\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> <title>Configuracion | Enlace Inalambrico</title> <style> html { background-color: rgb(5, 5, 5); color: white; } .cfgPrincipal { display: flex; flex-direction: column; align-items: center; } .resaltado { color: rgb(0, 255, 0); } .btn { color: white; margin-top: 20px; border: 1px; border-color: white; padding: 4px 8px 4px 8px; transition-duration: 0.5s; text-shadow: rgba(0, 0, 0, 0.31) 2px 2px; } .btn:hover { cursor: pointer; } .connectBtn { background-color: rgb(0, 255, 0); } .disconnectBtn { background-color: red; } .locked { background-color: grey; border-color: grey; transition-duration: 0.5s; } .locked:hover { cursor: not-allowed; } .inputField { margin: 10px 0 10px 0; width: 700px; display: flex; justify-content: space-between; height: 40px; } .inputField input { width: 200px; } .inputField select { width: 200px; } .error { color: red; } </style> </head> <body> <section class=\"cfgPrincipal\"> <h1>Configuracion | Enlace inalámbrico</h1> <h2>IP Actual: <span class=\"resaltado ipAddress\">");
  client.print(espIP);
  client.print("</span></h2> <div class=\"inputField\"> <h2>SSID a conectar:</h2> <input type=\"text\" class=\"connectSSID\"> </div> <div class=\"inputField\"> <h2>Contraseña:</h2> <input type=\"password\" class=\"connectPass\"> </div> <div class=\"inputField\"> <h2>Servidor de destino:</h2> <input type=\"text\" class=\"connectServer\"> </div> <div class=\"inputField\"> <h2>Puerto:</h2> <input type=\"text\" class=\"connectPort\"> </div> <div class=\"inputField\"> <h2>Nombre de la variable:</h2> <input type=\"text\" class=\"inputName\"> </div> <div class=\"inputField\"> <h2>Pin de lectura:</h2> <select class=\"inputPin\"> <option value=\"32\">Pin 32</option> <option value=\"34\">Pin 34</option> <option value=\"36\">Pin 36</option> <option value=\"comm\">Comunicacion</option> </select> </div> <div class=\"inputField\"> <h2>Tipo de variable:</h2> <select class=\"inputType\"> <option value=\"digital\">Discreta</option> <option value=\"analog\">Analogica</option> </select> </div> <div class=\"inputField buttons\"><button class=\"btn connectBtn locked\">Conectar</button></div> <h2 class=\"connectStatus\"></h2> </section> <script> let connectFlag = false; let ipAddress = document.querySelector('.ipAddress').textContent; let inputsArray = document.getElementsByTagName('input'); let emptyInputLock = true; let analogCommLock = false; let mode = \"\"; fetch(`http://${ipAddress}/mode`) .then((response) => { console.log(response); return response.json(); }) .then((obj) => { mode = obj.mode; if (mode == \"1\") { let disconnectBtn = document.createElement('button'); disconnectBtn.classList.add('btn'); disconnectBtn.classList.add('disconnectBtn'); disconnectBtn.textContent = \"Desconectar\"; document.querySelector('.buttons').appendChild(disconnectBtn); disconnectBtn.addEventListener(\"click\", (e) => { e.preventDefault(); fetch(`http://${ipAddress}/disconnect`); }); } }); for (let input of inputsArray) { input.empty = true; input.addEventListener('input', (e) => { if (e.target.value != '') { e.target.empty = false; emptyInputLock = false; for (let input of inputsArray) { if (input.empty) { emptyInputLock = true; } } } else { e.target.empty = true; emptyInputLock = true; } if (emptyInputLock || analogCommLock) { document.querySelector('.connectBtn').classList.add('locked'); } else { document.querySelector('.connectBtn').classList.remove('locked'); } }) } let selectArray = document.getElementsByTagName('select'); for (let select of selectArray) { select.addEventListener('change', () => { if (document.querySelector('.inputType').value == 'analog' && document.querySelector('.inputPin').value == 'comm') { analogCommLock = true; } else { analogCommLock = false; } if (emptyInputLock || analogCommLock) { document.querySelector('.connectBtn').classList.add('locked'); } else { document.querySelector('.connectBtn').classList.remove('locked'); } }); } document.querySelector('.connectBtn').addEventListener(\"click\", (e) => { e.preventDefault(); if (!document.querySelector('.connectBtn').classList.contains('locked')) { let ssid = document.querySelector('.connectSSID').value; let password = document.querySelector('.connectPass').value; let server = document.querySelector('.connectServer').value; let port = document.querySelector('.connectPort').value; let name = document.querySelector('.inputName').value; let pin = document.querySelector('.inputPin').value; let type = document.querySelector('.inputType').value; let httpUrl = `http://${ipAddress}/connect?ssid=${ssid}&password=${password}&server=${server}&port=${port}&name=${name}&pin=${pin}&type=${type}`; fetch(httpUrl) .then(() => { document.querySelector('.connectStatus').innerHTML = '<span class=\"resaltado\">Conexión iniciada</span>'; }) .catch(() => { document.querySelector('.connectStatus').innerHTML = '<span class=\"error\">Fallo al conectar con el ESP32</span>'; }) } }); </script> </body> </html>");
}

String queryParse(String queryString, String targetParameter) {
  String readString = "";
  bool read = false;
  bool readValue = false;
  for (int i = 0; i < queryString.length(); i++) {
    char c = queryString.charAt(i);
    if (read && c != '&' && c != '=') {
      readString += c;
    }
    if (c == '?' || c == '&' || c == '\0') {
      if (!read) {
        read = true;
      } else if (readValue) {
        return readString;
      }
    } else if (c == '=' && read) {
      if (readString == targetParameter) {
        readValue = true;
        readString = "";
      } else {
        readString = "";
        read = false;
      }
    }
  }
  return readString;
}

void setupAPMode() {
  if (WifiMode == MODO_STA) { WiFi.disconnect(); }
  WiFi.mode(WIFI_AP);
  Serial.println("Configurando Access Point...");
  if (!WiFi.softAP(ssid, password)) {
    Serial.println("Error al iniciar el Access Point");
    while (1)
      ;  // muere el programa
  }
  WifiMode = MODO_AP;
  espIP = WiFi.softAPIP();
  Serial.print("IP del Access Point: ");
  Serial.println(espIP);
  Serial.print("SSID: ");
  Serial.println(ssid);
}

void setupStationMode(String SSID, String password) {
  WiFi.softAPdisconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, password);
  int maxTries = 20;
  int i = 0;
  Serial.println("Conectando a SSID en modo Station");
  while (WiFi.status() != WL_CONNECTED && (i < maxTries)) {
    Serial.print(".");
    i++;
    delay(500);
  }
  if (i == maxTries) {
    Serial.println("Error al conectar");
    Serial.println("Reiniciando modo Access Point");
    setupAPMode();
  } else {
    WifiMode = MODO_STA;
    Serial.println();
    Serial.println("Conectado");
    espIP = WiFi.localIP();
    Serial.print("IP adquirida: ");
    Serial.println(espIP);
    Serial.print("SSID de la red: ");
    Serial.println(WiFi.SSID());
  }
}



bool setupDataPin(String pin, String varType) {
  if (pin == "comm") {
    UARTMode = true;
    Serial.println("Conectado POR COMUNICACION UART");
    return DIGITAL_VAR;
  } else {
    UARTMode = false;
    dataPin = pin.toInt();
    pinMode(dataPin, INPUT);
    if (varType == "analog") {
      return ANALOG_VAR;
    } else {
      return DIGITAL_VAR;
    }
  }
}