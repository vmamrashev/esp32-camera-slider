#include <WiFi.h>
#include <Stepper.h>

const char* ssid = "ESP32-camera-slider";
const char* key = "123456789";

WiFiServer esp32Server(80);
String request;

const int in1 = 19;
const int in2 = 18;
const int in3 = 5;
const int in4 = 17;
const int stepsPerRevolution = 2048;
int stepperSpeed = 5;
int direction = 0; // 0 for clockwise, 1 for counterclockwise
int stepperON = 0; // 0 for off, 1 for on

Stepper stepperMotor(stepsPerRevolution, in1, in3, in2, in4);

void setup() {
  stepperMotor.setSpeed(stepperSpeed);        // set the speed of the stepper motor
  Serial.begin(115200);                       // Start the Serial communication to send messages to the computer
  Serial.print("Setting AP (Access Point)…"); // Start the access point
  WiFi.softAP(ssid, key);                     // WiFi.softAP(ssid); for open network
  IPAddress IP = WiFi.softAPIP();             // Get the IP address of the ESP32
  Serial.print("Access Point IP address: ");  
  Serial.println(IP);                         // Print the IP address to the Serial monitor
  esp32Server.begin();                        // Start the server
}

void loop(){
  WiFiClient client = esp32Server.available(); // Listen for incoming clients

  if (client) {                                // If a new client connects,
    Serial.println("New Client.");             // print a message out in the serial port
    String currentLine = "";                   // a String to contain data incoming from the client
    while (client.connected()) {               // loop while the client's connected
      if (client.available()) {                // if there is any data to read from the client,
        char c = client.read();                // read a char, then
        Serial.write(c);                       // print char in the serial monitor
        request += c;
        if (c == '\n') {                       // if the char is a newline character
          // if the current line is blank, that means you have two newline characters in a row.
          // that's the end of the client's request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>ESP32 camera slider</h1>");
            
            // Display current state, and ON/OFF buttons for GPIO 26  
            client.println("<p>GPIO 26 - State " + output26State + "</p>");
            // If the output26State is off, it displays the ON button       
            if (output26State=="off") {
              client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            // Display current state, and ON/OFF buttons for GPIO 27  
            client.println("<p>GPIO 27 - State " + output27State + "</p>");
            // If the output27State is off, it displays the ON button       
            if (output27State=="off") {
              client.println("<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the request variable
    request = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}