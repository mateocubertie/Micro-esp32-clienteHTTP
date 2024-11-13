/* Librería del Arduino Core que maneja la comunicación Wi-Fi */
#include <WiFi.h>
/* Librería para manejo de comunicaciones HTTP para ESP32 */
#include <HTTPClient.h>
/* Librería para manejar el modo AP de Wi-Fi del ESP32 */
#include <WiFiAP.h>
/* Librería para manejo de la comunicación serie */
#include <HardwareSerial.h>
/* Librerías para manejo de vectors ( = arrays dinámicos) */
#include <vector>
#include <stdlib.h>

/* Instanciamos un objeto de la clase HardwareSerial, que manejará la comunicación serie */
HardwareSerial serialComms(0);

/* Macros con constantes para configurar la comunicación serie */
#define RXPIN 3
#define TXPIN 1
#define BAUDRATE 9600
#define SERIAL_BUFFER_SIZE 64

/* GPIO del LED azul integrado en la placa */
#define BUILTIN_LED 2

/* Macros para modos posibles de funcionamiento del Wi-Fi */
#define MODO_AP false
#define MODO_STA true
/* Almacena el modo actual de funcionamiento del Wi-Fi */
bool wifiMode{};

/* Macros para los tipos de variables aceptados */
#define DIGITAL_VAR false
#define ANALOG_VAR true
/* Almacena el tipo de dato que el enlace fue configurado para leer */
bool dataType{};

/* Macros para los modos de asignación de IP que el usuario puede seleccionar */
#define IPMODE_DYNAMIC false
#define IPMODE_STATIC true
/* Almaceena el modo de asignación de IP configurado por el usuario */
bool IPMode{};

/* Determina si el usuario eligió recibir datos por comunicación serie via UART */
bool uartMode = false;

/* Varaibles que guardan los principales campos de configuración del enlace */

/* IP estática (solo usada si se elige asignación estática) */
String staticIP = "";
/* IP del servidor de destino */
String serverIP = "";
/* Puerto en que el servidor de destino espera recibir los datos */
String port = "";
/* Vector con los nombres elegidos para las variables */
std::vector<String> dataNames({});
/* Cantidad de variables esperadas (leídas del buffer UART) */
int8_t dataLength{};
/* Entero con el pin en que se leerá el dato ingresante */
int8_t dataPin{};
/* Delay entre lecturas/solicitudes POST (intervalo mínimo, una conexión inestable resultará en tiempos más extensos) */
int16_t postDelay{};
/* String leído del query enviado por la web de configuracion, contiene el pin (pero como caracter) */
String pinString = "";

/* SSID y contraseña de la red Wi-Fi iniciada por el ESP32 en modo AP */
const char *ssid = "esp32ap";
const char *password = "microesp32";

/* Dirección IP del ESP formateada como tal*/
IPAddress espIP;
/* Instancia un objeto WiFiServer para manejar la comunicación HTTP al funcionar en modo AP  */
WiFiServer server(80);
/* Instancia un objeto HTTPClient que maneja el envío de solicitudes desde el ESP32 */
HTTPClient http;

/* Clase para almacenar vectores y convertirlos en pares clave:valor en formato JSON  */
class JSON {
public:
  /* Vector que almacena las claves/keys */
  std::vector<String> keys{};
  /* Vector que almacena los valores/values */
  std::vector<String> values{};
  /* Convierte los vectores de claves y valores en un string con formato JSON */
  String stringToJSON() {
    /* Comienza con una llave abierta */
    String resultado = "{\n";
    /* Recorremos los vectores de claves y valores según la cantidad esperada de variables */
    for (int16_t i = 0; i < dataLength; i++) {
      if (i > 0) {
        /* Appendea una coma y un salto de línea (si no es la primera línea, para evitar error de formato en caso de 1 sola variable) */
        resultado += ",\n";
      }
      /* Appendea el par clave: valor entre comillas */
      resultado += '"' + keys[i] + "\": \"" + values[i] + '"';
    }
    /* Appendea un salto de línea y la llave de cierre */
    resultado += "\n}";
    Serial.println();
    Serial.println(resultado);
    /* Devuelve el string en formato JSON */
    return resultado;
  }
};



// Manda al cliente las lineas del HTML de la web
// (escribir el codigo en VSC, hacer Join Lines y pasarlo, pero hay que generar otro print donde haya variables del ESP32)
// Las comillas dobles hay que bypassearlas con una '\' antes (buscar y reemplazar en seleccion)
void printCfgHTML(WiFiClient client) {
  client.print("<!DOCTYPE html> <html lang=\"en\"> <head> <meta charset=\"UTF-8\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> <title>Configuracion | Enlace Inalambrico</title> <style> html { background-color: rgb(5, 5, 5); color: white; } .cfgPrincipal { display: flex; flex-direction: column; align-items: center; } .resaltado { color: rgb(0, 255, 0); } .btn { color: white; margin-top: 20px; border: 1px; border-color: white; padding: 4px 8px 4px 8px; transition-duration: 0.5s; text-shadow: rgba(0, 0, 0, 0.31) 2px 2px; } .btn:hover { cursor: pointer; } .connectBtn { background-color: rgb(0, 255, 0); } .disconnectBtn { background-color: red; } .locked { background-color: grey; border-color: grey; transition-duration: 0.5s; } .locked:hover { cursor: not-allowed; } .inputField { margin: 10px 0 10px 0; width: 700px; display: flex; justify-content: space-between; height: 40px; } .inputField input { width: 200px; } .inputField select { width: 200px; } .error { color: red; } </style> </head> <body> <section class=\"cfgPrincipal\"> <h1>Configuracion | Enlace inalámbrico</h1> <h2>IP Actual: <span class=\"resaltado ipAddress\">");
  /* Incrusta (en un lugar conveniente del código de la web) la IP actual del enlace */
  client.print(espIP);
  client.print("</span></h2> <div class=\"inputField\"> <h2>SSID a conectar:</h2> <input type=\"text\" class=\"connectSSID\"> </div> <div class=\"inputField\"> <h2>Contraseña:</h2> <input type=\"password\" class=\"connectPass\"> </div> <div class=\"inputField\"> <h2>Modo de asignacion de IP:</h2> <select class=\"connectMode\"> <option value=\"dynamic\">Dinamico</option> <option value=\"static\">Estatico</option> </select> </div> <div class=\"inputField\"> <h2>IP estatica del enlace:</h2> <input type=\"text\" class=\"connectStaticIP bypass\"> </div> <div class=\"inputField\"> <h2>Servidor de destino:</h2> <input type=\"text\" class=\"connectServer\"> </div> <div class=\"inputField\"> <h2>Puerto:</h2> <input type=\"text\" class=\"connectPort\"> </div> <div class=\"inputField\"> <h2>Pin de lectura:</h2> <select class=\"inputPin\"> <option value=\"32\">Pin 32</option> <option value=\"34\">Pin 34</option> <option value=\"36\">Pin 36</option> <option value=\"comm\">Comunicacion</option> </select> </div> <div class=\"inputField\"> <h2>Intervalo entre lecturas:</h2> <input type=\"text\" class=\"delay\"> </div> <div class=\"inputField\"> <h2>Tipo de variable:</h2> <select class=\"inputType\"> <option value=\"digital\">Discreta</option> <option value=\"analog\">Analogica</option> </select> </div> <div class=\"inputField multivariable\"> <h2>Cantidad de variables:</h2> <select class=\"dataLength\"> <option value=\"1\">1</option> <option value=\"2\">2</option> <option value=\"3\">3</option> <option value=\"4\">4</option> <option value=\"5\">5</option> <option value=\"6\">6</option> <option value=\"7\">7</option> <option value=\"8\">8</option> </select> </div> <div class=\"inputField multivariable bypass\"> <h2>Nombre de la variable 1:</h2> <input type=\"text\" class=\"dataName1\"> </div> <div class=\"inputField multivariable bypass\"> <h2>Nombre de la variable 2:</h2> <input type=\"text\" class=\"dataName2\"> </div> <div class=\"inputField multivariable bypass\"> <h2>Nombre de la variable 3:</h2> <input type=\"text\" class=\"dataName3\"> </div> <div class=\"inputField multivariable bypass\"> <h2>Nombre de la variable 4:</h2> <input type=\"text\" class=\"dataName4\"> </div> <div class=\"inputField multivariable bypass\"> <h2>Nombre de la variable 5:</h2> <input type=\"text\" class=\"dataName5\"> </div> <div class=\"inputField multivariable bypass\"> <h2>Nombre de la variable 6:</h2> <input type=\"text\" class=\"dataName6\"> </div> <div class=\"inputField multivariable bypass\"> <h2>Nombre de la variable 7:</h2> <input type=\"text\" class=\"dataName7\"> </div> <div class=\"inputField multivariable bypass\"> <h2>Nombre de la variable 8:</h2> <input type=\"text\" class=\"dataName8\"> </div> <div class=\"inputField monovariable bypass\"> <h2>Nombre de la variable 1:</h2> <input type=\"text\" class=\"dataName1\"> </div> <div class=\"inputField buttons\"><button class=\"btn connectBtn locked\">Conectar</button></div> <h2 class=\"connectStatus\"></h2> </section> <script> function checkEmptyInputs() { } let connectFlag = false; let ipAddress = document.querySelector('.ipAddress').textContent; let emptyInputLock = true; let analogCommLock = false; let camposMultivariable = []; for (let i = 0; i < 9; i++) { camposMultivariable.push(document.querySelector('.multivariable')); camposMultivariable[i].remove(); } let monovariable = document.querySelector('.monovariable'); let mode = \"\"; fetch(`http://${ipAddress}/mode`) .then((response) => { console.log(response); return response.json(); }) .then((obj) => { mode = obj.mode; if (mode == \"1\") { let disconnectBtn = document.createElement('button'); disconnectBtn.classList.add('btn'); disconnectBtn.classList.add('disconnectBtn'); disconnectBtn.textContent = \"Desconectar\"; document.querySelector('.buttons').appendChild(disconnectBtn); disconnectBtn.addEventListener('click', (e) => { e.preventDefault(); fetch(`http://${ipAddress}/disconnect`); }); } }); document.querySelector('.connectMode').addEventListener('change', (e) => { if (e.target.value == 'static') { document.querySelector('.connectStaticIP').classList.remove('bypass'); if (document.querySelector('.connectStaticIP').value == \"\") { document.querySelector('.connectBtn').classList.add('locked'); emptyInputLock = true; } } else { document.querySelector('.connectStaticIP').classList.add('bypass'); } }); document.querySelector('.inputPin').addEventListener('change', (e) => { if (e.target.value == \"comm\") { if (!document.body.contains(document.querySelector('.multivariable'))) { for (let campo of camposMultivariable) { document.querySelector('.cfgPrincipal').insertBefore(campo, document.querySelector('.buttons')); } document.querySelector(\".dataName1\").empty = false; emptyInputLock = false; for (let input of inputsArray) { if (input.empty && !input.classList.contains('bypass')) { emptyInputLock = true; } } if (emptyInputLock || analogCommLock) { document.querySelector('.connectBtn').classList.add('locked'); } else { document.querySelector('.connectBtn').classList.remove('locked'); } monovariable.remove(); } } else if (document.body.contains(document.querySelector('.multivariable'))) { for (let campo of camposMultivariable) { campo.remove(); } document.querySelector('.cfgPrincipal').insertBefore(monovariable, document.querySelector('.buttons')); } console.log(document.body.contains(document.querySelector('.multivariable'))); }); let inputsArray = document.getElementsByTagName('input'); for (let input of inputsArray) { if (input.classList.contains('bypass')) { input.empty = false; } else { input.empty = true; } input.addEventListener('input', (e) => { if (e.target.value != '') { e.target.empty = false; emptyInputLock = false; for (let input of inputsArray) { if (input.empty && !input.classList.contains('bypass')) { emptyInputLock = true; } } } else if (!input.classList.contains('bypass')) { e.target.empty = true; emptyInputLock = true; } if (emptyInputLock || analogCommLock) { document.querySelector('.connectBtn').classList.add('locked'); } else { document.querySelector('.connectBtn').classList.remove('locked'); } }); } let selectArray = document.getElementsByTagName('select'); for (let select of selectArray) { select.addEventListener('change', () => { if (document.querySelector('.inputType').value == 'analog' && document.querySelector('.inputPin').value == 'comm') { analogCommLock = true; } else { analogCommLock = false; } if (emptyInputLock || analogCommLock) { document.querySelector('.connectBtn').classList.add('locked'); } else { document.querySelector('.connectBtn').classList.remove('locked'); } }); } document.querySelector('.connectBtn').addEventListener(\"click\", (e) => { e.preventDefault(); if (!document.querySelector('.connectBtn').classList.contains('locked')) { let ssid = document.querySelector('.connectSSID').value; let password = document.querySelector('.connectPass').value; let modeSelect = document.querySelector('.connectMode').value; let staticIP = document.querySelector('.connectStaticIP').value; let server = document.querySelector('.connectServer').value; let port = document.querySelector('.connectPort').value; let pin = document.querySelector('.inputPin').value; let type = document.querySelector('.inputType').value; let delay = document.querySelector('.delay').value; let httpUrl = `http://${ipAddress}/connect?ssid=${ssid}&password=${password}&ipmode=${modeSelect}&staticIP=${staticIP}&server=${server}&port=${port}&pin=${pin}&type=${type}&delay=${delay}`; if (pin == \"comm\") { let dataLength = document.querySelector('.dataLength').value; httpUrl += `&length=${dataLength}`; for (let i = 1; i <= dataLength; i++) { httpUrl += `&dataName${i}=${document.querySelector(`.dataName${i}`).value}`; } } else { httpUrl += `&length=1&dataName1=${document.querySelector('.dataName1').value}`; } console.log(httpUrl); fetch(httpUrl) .then(() => { document.querySelector('.connectStatus').innerHTML = '<span class=\"resaltado\">Conexión iniciada</span>'; }) .catch(() => { document.querySelector('.connectStatus').innerHTML = '<span class=\"error\">Fallo al conectar con el ESP32</span>'; }) } }); </script> </body> </html>");
}

/* Parsea (analiza) la consulta recibida mediante solicitud GET para obtener los parámetros buscados */
String queryParse(String queryString, String targetParameter) {
  String readString = "";
  /* Habilita la lectura */
  bool read = false;
  /* Indica que se esta leyendo el valor del parametro buscado */
  bool readValue = false;
  /* Lee la consulta caracter por caracter */
  for (int i = 0; i < queryString.length(); i++) {
    /* Almacena el caracter actual */
    char c = queryString.charAt(i);
    /* Lee todos los caracteres, menos los que separan las claves de los valores (=) o los objetos & */
    if (read && c != '&' && c != '=') {
      readString += c;
    }
    if (c == '?' || c == '&' || c == '\0') {
      if (!read) {
        read = true;
      } 
      /* Si se esta leyendo el valor del parametro buscado, termina y lo devuelve */
      else if (readValue) {
        return readString;
      }
    } else if (c == '=' && read) {
      /* Si la clave leída coincide con el parametro buscado, comienza a leer el valor */
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

/* Configura el ESP32 en modo AP */
void setupAPMode() {
  /* Si esta en modo estación, desconecta el Wi-Fi */
  if (wifiMode == MODO_STA) { WiFi.disconnect(); }
  /* Establece el modo AP */
  WiFi.mode(WIFI_AP);
  Serial.println("\nConfigurando Access Point...");
  /* Intenta conectar, y si falla, intenta nuevamente (NO DEBERIA FALLAR!) */
  if (!WiFi.softAP(ssid, password)) {
    Serial.println("FALLA CRITICA: Error al iniciar el Access Point");
    setupAPMode();
  }
  /* Actualiza el modo actual de funcionamiento del Wi-Fi */
  wifiMode = MODO_AP;
  /* Actualiza la IP del enlace */
  espIP = WiFi.softAPIP();
  Serial.print("IP del Access Point: ");
  Serial.println(espIP);
  Serial.print("SSID: ");
  Serial.println(ssid);
}

/* Configura el modo estación */
void setupStationMode(String SSID, String password) {
  /* Desactiva la red Wi-Fi propia del enlace */
  WiFi.softAPdisconnect();
  /* Configura el Wi-Fi en modo estación */
  WiFi.mode(WIFI_STA);
  /* Inicia la conexión Wi-Fi */
  WiFi.begin(SSID, password);
  /* Intenta conectarse un máximo de 20 veces (1 vez cada 500ms) */
  int maxTries = 20;
  int i = 0;
  Serial.println("Conectando a SSID en modo Station");
  while (WiFi.status() != WL_CONNECTED && (i < maxTries)) {
    Serial.print(".");
    i++;
    delay(500);
  }
  /* Si falla al conectarse a la red configurada, vuelve al modo AP */
  if (i == maxTries) {
    Serial.println("Error al conectar");
    Serial.println("Reiniciando modo Access Point");
    setupAPMode();
  } else {
    wifiMode = MODO_STA;
    /* Si el modo de asignación de IP es dinámica, permanece conectado */
    if (IPMode != IPMODE_STATIC) {
      Serial.println();
      Serial.println("Conectado");
      espIP = WiFi.localIP();
      Serial.print("IP adquirida: ");
      Serial.println(espIP);
      Serial.print("SSID de la red: ");
      Serial.println(SSID);
    }
  }
  /* Si el modo de asignación de IP es estática, se conecta con IP dinámica, extrae el gateway, subred y DNS, y vuelve */
  if (IPMode == IPMODE_STATIC) {

    IPAddress IP;
    /* Trae la IP estática desde el string obtenido de la query */
    IP.fromString(staticIP);
    /* Guarda el gateway, subnet mask y DNS */
    IPAddress gateway = WiFi.gatewayIP();
    IPAddress subred = WiFi.subnetMask();
    IPAddress dnsIP = WiFi.dnsIP();
    /* Se desconecta */
    WiFi.disconnect();
    /* Se reconfigura con IP estática y cargando los datos extraidos (necesarios para usar IP estatica) */
    if(!WiFi.config(IP, gateway, subred, dnsIP, dnsIP)) {
      Serial.println("Configuración de IP estatica fallida");
      Serial.println("Reiniciando modo Access Point");
      setupAPMode();
    }
    /* Se reconecta */
    WiFi.begin(SSID, password);
    Serial.println();
      Serial.println("Conectado");
      espIP = WiFi.localIP();
      Serial.print("IP adquirida: ");
      Serial.println(espIP);
      Serial.print("SSID de la red: ");
      Serial.println(targetSSID);
  }
}

/* Configura el pin indicado para el tipo de lectura configurada */
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
  /* Configura la comunicación serie */ 
  serialComms.setRxBufferSize(SERIAL_BUFFER_SIZE);
  serialComms.begin(BAUDRATE, SERIAL_8N1, RXPIN, TXPIN);
  // Genera una interrupcion por cada byte enviado para leerlo
  setupAPMode();
  server.begin();
  Serial.println();
  Serial.println("Servidor iniciado");
  /* Configura el pin del LED azul integrado */
  pinMode(BUILTIN_LED, OUTPUT);
}

void loop() {
  digitalWrite(BUILTIN_LED, 0);
  /* Si el enlace está en modo AP, se enciende el LED azul */
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(BUILTIN_LED, 1);
  }
  WiFiClient client = server.accept();  // listen for incoming clients
    // Rutina de manejo de comunicación HTTP
  if (client) {          
     /* Si hay un cliente conectado al servidor, guarda en un string los datos ingresantes*/
    Serial.println("Cliente conectado");
    String currentLine = "";
    bool connectFlag = false;
    bool disconnectFlag = false;
    bool modeFlag = false;
    bool parameterReading = false;
    bool sendingData = false;
    String connectParams = "?";
    String targetSSID = "";
    String targetPass = "";
    /* Mientras haya un cliente conectado, lee los bytes que llegan */
    while (client.connected()) {  // loop while the client's connected
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        /* Si es una consulta connect (enviada por la web cuando el usuario termina la configuracion), comienza a leer los parametros de la configuracion */
        if (currentLine == "GET /connect?") {
          connectFlag = true;
          parameterReading = true;
        } 
        /* Si se solicita la desconexion (estando en modo STA), ejecuta la rutina de desconexion */
        else if (currentLine == "GET /disconnect" && wifiMode == MODO_STA) {
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
        if (c == '\n') {  // Si el byte es un salto de linea
          /* Si la linea actual estaba en blanco, hay 2 saltos de linea seguidos, indicando el fin del cuerpo de la solicitud */
          if (currentLine.length() == 0) {
            /* Envia una respuesta (200 -> OK) */
            client.println("HTTP/1.1 200 OK");
            /* Si se esta consultando el modo actual de funcionamiento */
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
            // Termina la respuesta con otra linea en blanco
            client.println();
            if (disconnectFlag) {
              disconnectFlag = false;
              setupAPMode();
            }
            break;
          } else {
            currentLine = "";
          }
        } 
        /* Añade el caracter a la lectura actual (si no es un carriage return) */
        else if (c != '\r') 
          currentLine += c;
        }
      }
    }
    /* Si la consulta es /connect?, se parsean los parametros para obtener la configuracion */
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
        postDelay = queryParse(connectParams, "delay").toInt();
        /* Limpia el vector de los nombres de datos */
        dataNames.clear();
        /* Por cada variable informada en la configuracion, parsea el nombre de la variable */
        for (int8_t i = 1; i <= dataLength; i++) {
          dataNames.push_back(queryParse(connectParams, String("dataName" + String(i))));
          Serial.println(String("Nombre dato: " + dataNames[i]));
        }
        pinString = queryParse(connectParams, "pin");
        dataType = setupDataPin(pinString, queryParse(connectParams, "type"));
        /* Configura el enlace en modo Station */
        setupStationMode(targetSSID, targetPass);
        connectParams = "?";
        connectFlag = false;
      }
    }

    /* Cierra la conexion */
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
    /* Inicializa el JSON que va a enviar con la solicitud POST */
    JSON lectura{};
    /* Mete en el vector de claves del JSON los nombres de las variables, y strings vacios para los valores */
    for (uint8_t index = 0; index < dataLength; index++) {
      lectura.keys.push_back(dataNames[index]);
      lectura.values.push_back("");
    }
    /* Si esta recibiendo datos por comunicacion */
    if (uartMode) {
      /* String en que se lee el buffer completo */
      String bufferString = "";
      /* String que lee solo el último paquete */
      String readString = "";
      bool readFlag = false;
      bool packageFlag = false;
      uint8_t packageCount = 0;
      uint8_t currentPackage = 0;
      /* Numero de variable */
      uint8_t varIndex = 0;
      /* Lee todos los bytes disponibles  */
      while (serialComms.available()) {
        char readByte = serialComms.read();
        bufferString = bufferString + readByte;
        if (readByte == '{') {
          packageFlag = true;
        }
        /* Cuenta la cantidad de paquetes completos que le llegaron por UART */
        if (readByte == '}' && packageFlag) {
          packageCount++;
          packageFlag = false;
        }
      }
      /* Index del while */
      uint16_t i = 0;
      /* Lee el string formado con el buffer */
      while (i < bufferString.length()) {
        char c = bufferString.charAt(i);
        /* Determina el nro. de paquete actual */
        if (c == '{') {
          currentPackage++;
        }
        if (c == '}' && readFlag) {
          break;
        }
        /* Si esta leyendo valores */
        if (readFlag) {
          /* Si detecta el final de un valor, incrementa el numero de variable */
          if (c == ';') {
            varIndex++;
            if (varIndex+1 > dataLength) {
              break;
            }
          }
          /* Sino, guarda el caracter en el valor de la variable actual */
          else {
            lectura.values[varIndex] += c;
          }
        }
        /* Si el paquete actual es el ultimo, comienza a leer valores */
        if (currentPackage == packageCount) {
          readFlag = true;
        }
        i++;
      }
    } 
    /* Si se configuro una lectura analogica */
    else if (dataType == ANALOG_VAR) {
      /* Guarda en la variable 0 el valor leído mediante ADC del pin (y lo convierte primero en string) */
      lectura.values[0] = String(analogRead(dataPin), DEC);
      Serial.println(analogRead(dataPin));
    } 
    /* Si se configuro una lectura discreta, lee el pin y guarda el valor en la variable 0*/
    else {
      if (digitalRead(dataPin)) {
        lectura.values[0] = String("true");
      } else {
        lectura.values[0] = String("false");
      }
    }
    Serial.print("Dato recibido: ");
    Serial.println(lectura.stringToJSON());
    /* Envia el JSON al servidor en el cuerpo de una solicitud POST */
    /* Si no recibe respuesta en 5 segundos, el LED azul se enciende por medio segundo*/
    if(http.POST(lectura.stringToJSON()) != 200) {
      digitalWrite(BUILTIN_LED, 1);
      delay(500);
    }
  }
  /* Si se encuentra en modo Station, utiliza el delay configurado */
  if (wifiMode == MODO_STA) {
    delay(postDelay);
  }
  else {
    /* En modo AP, el delay por defecto es de 500ms */
    delay(500);
  }
}
