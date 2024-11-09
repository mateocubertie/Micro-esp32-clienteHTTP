#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiAP.h>
#include <HardwareSerial.h>
#include <vector>
#include <stdlib.h>

HardwareSerial serialComms(0);

#define RXPIN 3
#define TXPIN 1
#define BAUDRATE 9600
#define SERIAL_BUFFER_SIZE 64
#define BUILTIN_LED 2
#define MODO_AP false
#define MODO_STA true
bool wifiMode{};

#define DIGITAL_VAR false
#define ANALOG_VAR true
bool dataType{};

#define IPMODE_DYNAMIC false
#define IPMODE_STATIC true
bool IPMode{};

bool uartMode = false;

String staticIP = "";
String serverIP = "";
String port = "";
std::vector<String> dataNames({});
int8_t dataLength{};
int8_t dataPin{};
String pinString = "";


const char *ssid = "esp32ap";
const char *password = "microesp32";

class JSON {
public:
  std::vector<String> keys{};
  std::vector<String> values{};
  String stringToJSON() {
    String resultado = "{\n";
    for (int16_t i = 0; i < dataLength; i++) {
      if (i > 0) {
        resultado += ",\n";
      }
      resultado += '"' + keys[i] + "\": \"" + values[i] + '"';
    }
    resultado += "\n}";
    Serial.println();
    Serial.println(resultado);
    return resultado;
  }
};

IPAddress espIP;
WiFiServer server(80);
HTTPClient http;

// Manda al cliente las lineas del HTML de la web
// (escribir el codigo en VSC, hacer Join Lines y pasarlo, pero hay que generar otro print donde haya variables del ESP32)
// Las comillas dobles hay que bypassearlas con '\' (buscar y reemplazar en seleccion)
void printCfgHTML(WiFiClient client) {
  client.print("<!DOCTYPE html> <html lang=\"en\"> <head> <meta charset=\"UTF-8\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> <title>Configuracion | Enlace Inalambrico</title> <style> html { background-color: rgb(5, 5, 5); color: white; } .cfgPrincipal { display: flex; flex-direction: column; align-items: center; } .resaltado { color: rgb(0, 255, 0); } .btn { color: white; margin-top: 20px; border: 1px; border-color: white; padding: 4px 8px 4px 8px; transition-duration: 0.5s; text-shadow: rgba(0, 0, 0, 0.31) 2px 2px; } .btn:hover { cursor: pointer; } .connectBtn { background-color: rgb(0, 255, 0); } .disconnectBtn { background-color: red; } .locked { background-color: grey; border-color: grey; transition-duration: 0.5s; } .locked:hover { cursor: not-allowed; } .inputField { margin: 10px 0 10px 0; width: 700px; display: flex; justify-content: space-between; height: 40px; } .inputField input { width: 200px; } .inputField select { width: 200px; } .error { color: red; } </style> </head> <body> <section class=\"cfgPrincipal\"> <h1>Configuracion | Enlace inalámbrico</h1> <h2>IP Actual: <span class=\"resaltado ipAddress\">");
  client.print(espIP);
  client.print("</span></h2> <div class=\"inputField\"> <h2>SSID a conectar:</h2> <input type=\"text\" class=\"connectSSID\"> </div> <div class=\"inputField\"> <h2>Contraseña:</h2> <input type=\"password\" class=\"connectPass\"> </div> <div class=\"inputField\"> <h2>Modo de asignacion de IP:</h2> <select class=\"connectMode\"> <option value=\"dynamic\">Dinamico</option> <option value=\"static\">Estatico</option> </select> </div> <div class=\"inputField\"> <h2>IP estatica del enlace:</h2> <input type=\"text\" class=\"connectStaticIP bypass\"> </div> <div class=\"inputField\"> <h2>Servidor de destino:</h2> <input type=\"text\" class=\"connectServer\"> </div> <div class=\"inputField\"> <h2>Puerto:</h2> <input type=\"text\" class=\"connectPort\"> </div> <div class=\"inputField\"> <h2>Pin de lectura:</h2> <select class=\"inputPin\"> <option value=\"32\">Pin 32</option> <option value=\"34\">Pin 34</option> <option value=\"36\">Pin 36</option> <option value=\"comm\">Comunicacion</option> </select> </div> <div class=\"inputField\"> <h2>Tipo de variable:</h2> <select class=\"inputType\"> <option value=\"digital\">Discreta</option> <option value=\"analog\">Analogica</option> </select> </div> <div class=\"inputField multivariable\"> <h2>Cantidad de variables:</h2> <select class=\"dataLength\"> <option value=\"1\">1</option> <option value=\"2\">2</option> <option value=\"3\">3</option> <option value=\"4\">4</option> <option value=\"5\">5</option> <option value=\"6\">6</option> <option value=\"7\">7</option> <option value=\"8\">8</option> </select> </div> <div class=\"inputField multivariable bypass\"> <h2>Nombre de la variable 1:</h2> <input type=\"text\" class=\"dataName1\"> </div> <div class=\"inputField multivariable bypass\"> <h2>Nombre de la variable 2:</h2> <input type=\"text\" class=\"dataName2\"> </div> <div class=\"inputField multivariable bypass\"> <h2>Nombre de la variable 3:</h2> <input type=\"text\" class=\"dataName3\"> </div> <div class=\"inputField multivariable bypass\"> <h2>Nombre de la variable 4:</h2> <input type=\"text\" class=\"dataName4\"> </div> <div class=\"inputField multivariable bypass\"> <h2>Nombre de la variable 5:</h2> <input type=\"text\" class=\"dataName5\"> </div> <div class=\"inputField multivariable bypass\"> <h2>Nombre de la variable 6:</h2> <input type=\"text\" class=\"dataName6\"> </div> <div class=\"inputField multivariable bypass\"> <h2>Nombre de la variable 7:</h2> <input type=\"text\" class=\"dataName7\"> </div> <div class=\"inputField multivariable bypass\"> <h2>Nombre de la variable 8:</h2> <input type=\"text\" class=\"dataName8\"> </div> <div class=\"inputField monovariable bypass\"> <h2>Nombre de la variable 1:</h2> <input type=\"text\" class=\"dataName1\"> </div> <div class=\"inputField buttons\"><button class=\"btn connectBtn locked\">Conectar</button></div> <h2 class=\"connectStatus\"></h2> </section> <script> function checkEmptyInputs() { } let connectFlag = false; let ipAddress = document.querySelector('.ipAddress').textContent; let emptyInputLock = true; let analogCommLock = false; let camposMultivariable = []; for (let i = 0; i < 9; i++) { camposMultivariable.push(document.querySelector('.multivariable')); camposMultivariable[i].remove(); } let monovariable = document.querySelector('.monovariable'); let mode = \"\"; fetch(`http://${ipAddress}/mode`) .then((response) => { console.log(response); return response.json(); }) .then((obj) => { mode = obj.mode; if (mode == \"1\") { let disconnectBtn = document.createElement('button'); disconnectBtn.classList.add('btn'); disconnectBtn.classList.add('disconnectBtn'); disconnectBtn.textContent = \"Desconectar\"; document.querySelector('.buttons').appendChild(disconnectBtn); disconnectBtn.addEventListener('click', (e) => { e.preventDefault(); fetch(`http://${ipAddress}/disconnect`); }); } }); document.querySelector('.connectMode').addEventListener('change', (e) => { if (e.target.value == 'static') { document.querySelector('.connectStaticIP').classList.remove('bypass'); if (document.querySelector('.connectStaticIP').value == \"\") { document.querySelector('.connectBtn').classList.add('locked'); emptyInputLock = true; } } else { document.querySelector('.connectStaticIP').classList.add('bypass'); } }); document.querySelector('.inputPin').addEventListener('change', (e) => { if (e.target.value == \"comm\") { if (!document.body.contains(document.querySelector('.multivariable'))) { for (let campo of camposMultivariable) { document.querySelector('.cfgPrincipal').insertBefore(campo, document.querySelector('.buttons')); } document.querySelector(\".dataName1\").empty = false; emptyInputLock = false; for (let input of inputsArray) { if (input.empty && !input.classList.contains('bypass')) { emptyInputLock = true; } } if (emptyInputLock || analogCommLock) { document.querySelector('.connectBtn').classList.add('locked'); } else { document.querySelector('.connectBtn').classList.remove('locked'); } monovariable.remove(); } } else if (document.body.contains(document.querySelector('.multivariable'))) { for (let campo of camposMultivariable) { campo.remove(); } document.querySelector('.cfgPrincipal').insertBefore(monovariable, document.querySelector('.buttons')); } console.log(document.body.contains(document.querySelector('.multivariable'))); }); let inputsArray = document.getElementsByTagName('input'); for (let input of inputsArray) { if (input.classList.contains('bypass')) { input.empty = false; } else { input.empty = true; } input.addEventListener('input', (e) => { if (e.target.value != '') { e.target.empty = false; emptyInputLock = false; for (let input of inputsArray) { if (input.empty && !input.classList.contains('bypass')) { emptyInputLock = true; } } } else if (!input.classList.contains('bypass')) { e.target.empty = true; emptyInputLock = true; } if (emptyInputLock || analogCommLock) { document.querySelector('.connectBtn').classList.add('locked'); } else { document.querySelector('.connectBtn').classList.remove('locked'); } }); } let selectArray = document.getElementsByTagName('select'); for (let select of selectArray) { select.addEventListener('change', () => { if (document.querySelector('.inputType').value == 'analog' && document.querySelector('.inputPin').value == 'comm') { analogCommLock = true; } else { analogCommLock = false; } if (emptyInputLock || analogCommLock) { document.querySelector('.connectBtn').classList.add('locked'); } else { document.querySelector('.connectBtn').classList.remove('locked'); } }); } document.querySelector('.connectBtn').addEventListener(\"click\", (e) => { e.preventDefault(); if (!document.querySelector('.connectBtn').classList.contains('locked')) { let ssid = document.querySelector('.connectSSID').value; let password = document.querySelector('.connectPass').value; let modeSelect = document.querySelector('.connectMode').value; let staticIP = document.querySelector('.connectStaticIP').value; let server = document.querySelector('.connectServer').value; let port = document.querySelector('.connectPort').value; let pin = document.querySelector('.inputPin').value; let type = document.querySelector('.inputType').value; let httpUrl = `http://${ipAddress}/connect?ssid=${ssid}&password=${password}&ipmode=${modeSelect}&staticIP=${staticIP}&server=${server}&port=${port}&pin=${pin}&type=${type}`; if (pin == \"comm\") { let dataLength = document.querySelector('.dataLength').value; httpUrl += `&length=${dataLength}`; for (let i = 1; i <= dataLength; i++) { httpUrl += `&dataName${i}=${document.querySelector(`.dataName${i}`).value}`; } } else { httpUrl += `&length=1&dataName1=${document.querySelector('.dataName1').value}`; } console.log(httpUrl); fetch(httpUrl) .then(() => { document.querySelector('.connectStatus').innerHTML = '<span class=\"resaltado\">Conexión iniciada</span>'; }) .catch(() => { document.querySelector('.connectStatus').innerHTML = '<span class=\"error\">Fallo al conectar con el ESP32</span>'; }) } }); </script> </body> </html>");
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
  if (wifiMode == MODO_STA) { WiFi.disconnect(); }
  WiFi.mode(WIFI_AP);
  Serial.println("\nConfigurando Access Point...");
  if (!WiFi.softAP(ssid, password)) {
    Serial.println("Error al iniciar el Access Point");
    while (1)
      ;  // muere el programa
  }
  wifiMode = MODO_AP;
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
    wifiMode = MODO_STA;
    if (IPMode != IPMODE_STATIC) {
      Serial.println();
      Serial.println("Conectado");
      espIP = WiFi.localIP();
      Serial.print("IP adquirida: ");
      Serial.println(espIP);
      Serial.print("SSID de la red: ");
      Serial.println(WiFi.SSID());
    }
  }
  if (IPMode == IPMODE_STATIC) {
    IPAddress IP;
    IP.fromString(staticIP);
    IPAddress gateway = WiFi.gatewayIP();
    IPAddress subred = WiFi.subnetMask();
    IPAddress dnsIP = WiFi.dnsIP();
    WiFi.disconnect();
    if(!WiFi.config(IP, gateway, subred, dnsIP, dnsIP)) {
      Serial.println("Configuración de IP estatica fallida");
      Serial.println("Reiniciando modo Access Point");
      setupAPMode();
    }
    WiFi.begin(SSID, password);
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
    uartMode = true;
    Serial.println("Conectado por comunicacion UART");
    return DIGITAL_VAR;
  } else {
    uartMode = false;
    dataPin = pin.toInt();
    pinMode(dataPin, INPUT);
    if (varType == "analog") {
      return ANALOG_VAR;
    } else {
      return DIGITAL_VAR;
    }
  }
}

void setup() {
  Serial.begin(9600);
  serialComms.setRxBufferSize(SERIAL_BUFFER_SIZE);
  serialComms.begin(BAUDRATE, SERIAL_8N1, RXPIN, TXPIN);
  // Genera una interrupcion por cada byte enviado para leerlo
  setupAPMode();
  server.begin();
  Serial.println();
  Serial.println("Servidor iniciado");
  pinMode(BUILTIN_LED, OUTPUT);
}

void loop() {
  digitalWrite(BUILTIN_LED, 0);
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(BUILTIN_LED, 1);
  }
  WiFiClient client = server.accept();  // listen for incoming clients
    // Rutina de manejo de comunicación HTTP
  if (client) {          
                                          // if you get a client,
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
        } else if (currentLine == "GET /disconnect" && wifiMode == MODO_STA) {
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
              client.print(wifiMode);
              client.println("\"}");
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
        IPMode = (queryParse(connectParams, "ipmode") == "static");
        if (IPMode == IPMODE_STATIC) {
          staticIP = queryParse(connectParams, "staticIP");
        }
        serverIP = queryParse(connectParams, "server");
        port = queryParse(connectParams, "port");
        dataLength = queryParse(connectParams, "length").toInt();
        dataNames.clear();
        for (int8_t i = 1; i <= dataLength; i++) {
          dataNames.push_back(queryParse(connectParams, String("dataName" + String(i))));
          Serial.println(String("Nombre dato: " + dataNames[i]));
        }
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

  // Rutina de envío de datos al servidor
  // Si esta en modo station (es decir, conectado a una SSID y con un servidor de destino)
  // manda la lectura con una peticion post
  if (wifiMode == MODO_STA) {
    Serial.println(String("http://" + serverIP + ":" + port + "/update"));
    http.begin(String("http://" + serverIP + ":" + port + "/update"));
    http.addHeader("Content-Type", "application/json");
    JSON lectura{};
    for (uint8_t index = 0; index < dataLength; index++) {
      lectura.keys.push_back(dataNames[index]);
      lectura.values.push_back("");
    }
    if (uartMode) {
      String bufferString = "";
      String readString = "";
      bool readFlag = false;
      bool packageFlag = false;
      uint8_t packageCount = 0;
      uint8_t currentPackage = 0;
      uint16_t i = 0;
      uint8_t varIndex = 0;
      while (serialComms.available()) {
        char readByte = serialComms.read();
        bufferString = bufferString + readByte;
        if (readByte == '{') {
          packageFlag = true;
        }
        if (readByte == '}' && packageFlag) {
          packageCount++;
          packageFlag = false;
        }
      }
      while (i < bufferString.length()) {
        char c = bufferString.charAt(i);
        if (c == '{') {
          currentPackage++;
        }
        if (c == '}' && readFlag) {
          break;
        }
        if (readFlag) {
          if (c == ';') {
            varIndex++;
            if (varIndex+1 > dataLength) {
              break;
            }
          }
          else {
            lectura.values[varIndex] += c;
          }
        }
        if (currentPackage == packageCount) {
          readFlag = true;
        }
        i++;
      }
    } else if (dataType == ANALOG_VAR) {
      lectura.values[0] = String(analogRead(dataPin), DEC);
      Serial.println(analogRead(dataPin));
    } else {
      if (digitalRead(dataPin)) {
        lectura.values[0] = String("true");
      } else {
        lectura.values[0] = String("false");
      }
    }
    Serial.print("Dato recibido: ");
    Serial.println(lectura.stringToJSON());
    if(http.POST(lectura.stringToJSON()) != 200) {
      digitalWrite(BUILTIN_LED, 1);
      delay(500);
    }
  }

  delay(500);
}
