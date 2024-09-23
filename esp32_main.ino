#include <WiFi.h>
#include <HTTPClient.h>
#include <NetworkClient.h>
#include <WiFiAP.h>


const char *ssid = "esp32ap";
const char *password = "microesp32";
IPAddress espIP;
NetworkServer server(80);
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Configurando Access Point...");

  if (!WiFi.softAP(ssid, password)) {
    Serial.println("Error al iniciar el Access Point");
    while (1)
      ;
  }

  espIP = WiFi.softAPIP();
  Serial.print("IP del Access Point: ");
  Serial.println(espIP);
  server.begin();

  Serial.println("Servidor iniciado");
}

void loop() {
  NetworkClient client = server.accept();  // listen for incoming clients

  if (client) {                           // if you get a client,
    Serial.println("Cliente conectado");  // print a message out the serial port
    String currentLine = "";              // make a String to hold incoming data from the client
    bool connectFlag = false;
    bool parameterReading = false;
    String connectParams = "?";
    String targetSSID = "";
    String targetPass = "";
    int dataPin {};
    String dataType = "";
    while (client.connected()) {          // loop while the client's connected
      if (client.available()) {           // if there's bytes to read from the client,
        char c = client.read();           // read a byte, then
        Serial.write(c);                  // print it out the serial monitor
        if (currentLine == "GET /connect?") {
            connectFlag = true;
            parameterReading = true;
        }
        if (parameterReading) {
            if (c == ' ') {
                parameterReading = false;
            }
            else {
                connectParams += c;
            }
        }
        if (c == '\n') {                  // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            printCfgHTML(client);

            // The HTTP response ends with another blank line:
            client.println();
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
            targetSSID = connectParamsParse(connectParams, "ssid");
            targetPass = connectParamsParse(connectParams, "password");
            dataPin = connectParamsParse(connectParams, "pin").toInt();
            dataType = connectParamsParse(connectParams, "type");
            Serial.println(targetSSID);
            Serial.println(targetPass);
            Serial.println(dataPin);
            Serial.println(dataType);
            setupESP32(targetSSID, targetPass, dataPin, dataType);
            connectParams = "?";
            connectFlag = false;
        }
    }
    // close the connection:
    client.stop();
    Serial.println("Cliente desconectado");
  }
}

// Funcion que manda al cliente las lineas del HTML de la web
// (escribir el codigo en VSC, hacer Join Lines y pasarlo, pero hay que generar otro print donde haya variables del ESP32)
// Las comillas dobles hay que bypassearlas con '\' (buscar y reemplazar en seleccion)
void printCfgHTML(NetworkClient client) {
  client.print("<!DOCTYPE html> <html lang=\"en\"> <head> <meta charset=\"UTF-8\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> <title>Configuracion | Enlace Inalambrico</title> <style> html { background-color: rgb(5, 5, 5); color: white; } .cfgPrincipal { display: flex; flex-direction: column; align-items: center; } .resaltado { color: rgb(0, 255, 0); } .connectBtn { color: white; margin-top: 20px; border: 1px; border-color: white; background-color: rgb(0, 255, 0); padding: 4px 8px 4px 8px; transition-duration: 0.5s; text-shadow: rgba(0, 0, 0, 0.31) 2px 2px; } .connectBtn:hover { cursor: pointer } .locked { background-color: grey; border-color: grey; transition-duration: 0.5s; } .locked:hover { cursor: not-allowed; } .inputField { margin: 10px 0 10px 0; width: 700px; display: flex; justify-content: space-between; height: 40px; } .inputField input { width: 200px; } .inputField select { width: 200px; } .error { color: red; } </style> </head> <body> <section class=\"cfgPrincipal\"> <h1>Configuracion | Enlace inalámbrico</h1> <h2>IP Actual: <span class=\"resaltado ipAddress\">");
  client.print(espIP);
  client.print("</span></h2> <div class=\"inputField\"> <h2>SSID a conectar:</h2> <input type=\"text\" class=\"connectSSID\"> </div> <div class=\"inputField\"> <h2>Contraseña:</h2> <input type=\"password\" class=\"connectPass\"> </div> <div class=\"inputField\"> <h2>Pin de lectura:</h2> <select class=\"inputPin\"> <option value=\"gpio0\">Pin 0</option> <option value=\"gpio2\">Pin 2</option> <option value=\"gpio4\">Pin 4</option> <option value=\"comm\">Comunicacion</option> </select> </div> <div class=\"inputField\"> <h2>Tipo de variable:</h2> <select class=\"inputType\"> <option value=\"digital\">Discreta</option> <option value=\"analog\">Analogica</option> </select> </div> <div class=\"inputField\"><button class=\"connectBtn locked\" style=\"justify-self: center;\">Conectar</button></div> <h2 class=\"connectStatus\"></h2> </section> <script> let connectFlag = false; let ipAddress = document.querySelector('.ipAddress').textContent; let inputsArray = document.getElementsByTagName('input'); let emptyInputLock = true; let analogCommLock = false; for (let input of inputsArray) { input.empty = true; input.addEventListener('input', (e) => { if (e.target.value != '') { e.target.empty = false; emptyInputLock = false; for (let input of inputsArray) { if (input.empty) { emptyInputLock = true; } } } else { e.target.empty = true; emptyInputLock = true; } if (emptyInputLock || analogCommLock) { document.querySelector('.connectBtn').classList.add('locked'); } else { document.querySelector('.connectBtn').classList.remove('locked'); } }) } let selectArray = document.getElementsByTagName('select'); for (let select of selectArray) { select.addEventListener('change', () => { if (document.querySelector('.inputType').value == 'analog' && document.querySelector('.inputPin').value == 'comm') { analogCommLock = true; } else { analogCommLock = false; } if (emptyInputLock || analogCommLock) { document.querySelector('.connectBtn').classList.add('locked'); } else { document.querySelector('.connectBtn').classList.remove('locked'); } }) } document.querySelector('.connectBtn').addEventListener(\"click\", (e) => { e.preventDefault(); if (!document.querySelector('.connectBtn').classList.contains('locked')) { let ssid = document.querySelector('.connectSSID').value; let password = document.querySelector('.connectPass').value; let pin = document.querySelector('.inputPin').value; let type = document.querySelector('.inputType').value; let httpUrl = `http://${ipAddress}/connect?ssid=${ssid}&password=${password}&pin=${pin}&type=${type}`; fetch(httpUrl) .then(() => { document.querySelector('.connectStatus').innerHTML = '<span class=\"resaltado\">Conexión iniciada</span>'; }) .catch(()=>{ document.querySelector('.connectStatus').innerHTML = '<span class=\"error\">Fallo al conectar con el ESP32</span>'; }) } }) </script> </body> </html>");
}

String connectParamsParse(String parameterString, String targetParameter) {
    String readString = "";
    bool read = false;
    bool readValue = false;
    Serial.println(parameterString.length());
    for (int i = 0; i < parameterString.length(); i++) {
        char c = parameterString.charAt(i);
        if (read && c != '&' && c != '=') {
            readString += c;
        }
        if (c == '?' || c == '&' || c == '\0') {
            if (!read) {read = true;}
            else if (readValue) {
                return readString;
            }
        }
        else if (c == '=' && read) {
            Serial.println(readString);
            if (readString == targetParameter) {
                Serial.println("Se dio");
                readValue = true;
                readString = "";
            }
            else {
                readString = "";
                read = false;
            }
        }
    }
    return readString;
}

void setupESP32(String SSID, String password, int inputPin, String inputType) {
    pinMode(inputPin, INPUT);
    WiFi.softAPdisconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, password);
    int numberOfTries = 20;
    int i = 0;
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
        Serial.println();
        Serial.println("Conectado");
        espIP = WiFi.localIP();
        Serial.println(espIP);
        Serial.println(WiFi.SSID());
}
