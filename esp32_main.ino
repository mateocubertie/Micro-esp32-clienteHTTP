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
    while (client.connected()) {          // loop while the client's connected
      if (client.available()) {           // if there's bytes to read from the client,
        char c = client.read();           // read a byte, then
        Serial.write(c);                  // print it out the serial monitor
        if (c == '\n') {                  // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            printWebHTML(client);

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
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}

// Funcion que manda al cliente las lineas del HTML de la web
// (escribir el codigo en VSC y pasarlo, pero hay que generar otro print donde hay variables)
// Las comillas dobles hay que bypassearlas con '\' (buscar y reemplazar en seleccion)
void printWebHTML(NetworkClient client) {
  // the content of the HTTP response follows the header:
//   client.print("<!DOCTYPE html> <html lang=\"en\"><head>");
//   client.print("<meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
//   client.print("<title>Configuracion | Enlace Inalambrico</title><style>");
//   client.print(" @import url('https://fonts.googleapis.com/css2?family=Roboto:ital,wght@0,100;0,300;0,400;0,500;0,700;0,900;1,100;1,300;1,400;1,500;1,700;1,900&display=swap');");
//   client.print("html { background-color: rgb(18, 18, 18); color: white; font-family: \"Roboto\", sans-serif; }");
//   client.print(".cfgPrincipal{ display: flex; flex-direction: column;justify-content: center; }");
//   client.print(".resaltado{color:rgb(0, 230, 0)}");
//   client.print("</style></head><body><section class=\"cfgPrincipal\">");
//   client.print("<h1>Configuracion | Enlace inalámbrico</h1>");
//   client.print("<h2>IP Actual: <span class=\"resaltado\"></span></h2>");
//   client.print("<h3>");
  client.print("<!DOCTYPE html> <html lang=\"en\"><head> \
    <meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> \
    <title>Configuracion | Enlace Inalambrico</title><style> \
    @import url('https://fonts.googleapis.com/css2?family=Roboto:ital,wght@0,100;0,300;0,400;0,500;0,700;0,900;1,100;1,300;1,400;1,500;1,700;1,900&display=swap'); \
    html { background-color: rgb(18, 18, 18); color: white; font-family: \"Roboto\", sans-serif; } \
    .cfgPrincipal{ display: flex; flex-direction: column;align-items: center; } \
    .resaltado{ color:rgb(0, 230, 0);} \
    </style></head><body><section class=\"cfgPrincipal\"> \
    <h1>Configuracion | Enlace inalámbrico</h1> \
    <h2>IP Actual: <span class=\"resaltado\"> \
    <h3>"); 
  client.print(espIP);
  client.print("</span></h2></h3></section></body></html>");
    // client.print("</h3>");
    // client.print("</section></body></html>");
}

