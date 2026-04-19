//Remote Remote Coax Switch
//
//  By Brad Mitchell N8YG April 4 2022
// Revised with permission of Brad Mitchell by Paul Stumpf, KE2SI for the use by The Antique Wireless Association
// February 18, 2025 
//  2/25/2026 - T. Vaccaro W2CRG - updated button behavior; added phys button mode; reboot via web.  v0.9
//  3/22/2026 - T. Vaccaro - cleaned up transition order in phys button mode; removed on/off state above web buttons  v0.91
//  3/27/2026 - T. Vaccaro - removed on/off indication from OLED display v.1.0

// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include <WiFiManager.h> 

//display requirements
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//options to define
#define SW_VER "v1.0"  // 1.0 - release
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//end of display requirements

// Set web server port number to 32763
WiFiServer server(32763);
WiFiClient client;

// Variable to store the HTTP request
String header;
//Variable to store IP 
IPAddress MYIP;

// Auxiliar variables to store the current output state
String J5_Ant= "off";
String J6_Ant = "off";
String J3_Ant = "on";
String J4_Ant = "off";
String J1orJ2_Ant = "Remote";
//your definitions for your antennas
//define each according to what you have
//J1 and J2 accessible using accessory external relay board.
//Hook J3 to the rig J1 to your HF antenna coax cable feeding the remote switch
//and J2 to your 6 Meter antenna

String J1_name = "Remote Antennas";
String J2_name = "Local Antenna";
String J3_name = " Antenna 3";
String J4_name = " Antenna 4";
String J5_name = " Antenna 1";
String J6_name = " Antenna 2";

// Assign output variables to GPIO pins
const int Output13 = 13;
const int Output0 = 0;
const int Output12 = 12;
//const int Output0 = 0;
const int Output15 = 15;
const int Input14 = 14;  //pushbutton

int prevPbState = 1;
int pbState = 1;
int physButtonMode = 0;
int rebootFlag = 0;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

//========================================================================================================                    
void setup() {
  Serial.begin(115200);
 
  // check for long button press to allow exclusive control from physical button 
  // (input14) - this disables the web interface.
  currentTime = millis();
  previousTime = currentTime;
  while (currentTime - previousTime <= timeoutTime) {


      currentTime = millis(); 
      pbState = digitalRead(Input14);
    
      if (pbState < prevPbState)
      {
        Serial.println("setup: input14 pressed, entering physButtonMode");
        pbState = 1;
        physButtonMode = 1;
        // delay to avoid immediate coax switching
        delay(2000);
        break;
      }
  }

  //===========================================================================                    
  //if (!physButtonMode){
      WiFiManager wifiManager;
    // wifiManager.resetSettings();
      //set custom ip for portal
      
    //COMMENT THE NEXT LINE if you want a DYNAMICALLY ALLOCATED IP ADDRESS
      //wifiManager.setSTAStaticIPConfig(IPAddress(192,168,4,73), IPAddress(192,168,4,1), IPAddress(255,255,255,0)); 
    //If you want to use a static IP, then you need to use the first entry as your IP the second as your gateway the third leave as is. 
      //edit the following line with your SSID that you want to connect up to. 
      //using your callsign is good and a simple password is good as this is only connecting to the 
      //ESP8266 so that you can enter your ssid and password for your home wifi network 
      wifiManager.autoConnect("AWAPROJ", "PASSWORD");
  //  }

    // Initialize the output variables as outputs
    pinMode(Output13, OUTPUT);
    pinMode(Output0, OUTPUT);
    pinMode(Output12, OUTPUT);
    pinMode(Output15, OUTPUT);
    
    // Set outputs to HIGH
    digitalWrite(Output13, HIGH);
    digitalWrite(Output0, HIGH);
    digitalWrite(Output12, HIGH);
    digitalWrite(Output15, HIGH);

    // Inititialize pushbutton switch
    pinMode(Input14, INPUT_PULLUP);
    //digitalWrite(Input14, HIGH);

    //if (!physButtonMode){
      server.begin();
      MYIP =WiFi.localIP();
    //  }

    //DISPLAY INIT
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) { // Address 0x3D for 128x64
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
      }

    delay(2000);
    display.clearDisplay();

    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(25, 0);
    display.println (MYIP);
    display.setCursor(10, 30);
    // Display static text
    display.setTextSize(2);
    display.println(" AWA RCS\n  "+(String)SW_VER);
    display.display();
    //END DISPLAY INIT

    Serial.println("AWA Remote Coax Switch SW Version "+ String(SW_VER));
}  //  init

//==============================================================================================================                    
  void clientUpdate(){

  client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;

    //==============================================================================================================                    
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis(); 
   

      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
//==================================================================J5 Antenna Selection selection
            if (header.indexOf("GET /0/off") >= 0) {
              Serial.println("Turning on "+J5_name);
 
              J5_Ant= "on";
              J6_Ant = "off";
              J3_Ant = "off";
              J4_Ant = "off";
              J1orJ2_Ant = "Remote";
              digitalWrite(Output15, LOW);
              digitalWrite(Output13, LOW);
              digitalWrite(Output0, LOW); 
              digitalWrite(Output12, LOW);
              //Display 
              display.clearDisplay();
              display.setTextSize(2);
              display.setCursor(0, 27);
              display.println(J5_name);
              display.println(" "+J1orJ2_Ant);
              display.display(); 
              //END DISPLAY  
              
            }


//==================================================================J6 Antenna selection
            if (header.indexOf("GET /1/off") >= 0) {
              Serial.println("Turning on " +J6_name);

                J6_Ant = "on";
                J5_Ant= "off";
                J3_Ant = "off";
                J4_Ant = "off";
                J1orJ2_Ant = "Remote";
              digitalWrite(Output15, LOW);
              digitalWrite(Output13, HIGH); //K1
              digitalWrite(Output0, LOW); 
              digitalWrite(Output12, LOW);
              //Display 
              display.clearDisplay();
              display.setTextSize(2);
              display.setCursor(0, 27);
              display.println(J6_name);
              display.println(" "+J1orJ2_Ant);
              display.display(); 
              //END DISPLAY   
            }

//==================================================================J3 Antenna selection
            if (header.indexOf("GET /2/off") >= 0) {
+              Serial.println("Turning on " +J3_name);
           
              

                J3_Ant = "on";
                J6_Ant = "off";
                J5_Ant= "off";
                J4_Ant = "off";
                J1orJ2_Ant = "Remote";
              digitalWrite(Output15, LOW);                
              digitalWrite(Output13, LOW);//K1
              digitalWrite(Output0, HIGH); //K2
              digitalWrite(Output12, HIGH);//K3
              //Display 
              display.clearDisplay();
              display.setTextSize(2);
              display.setCursor(0, 27);
              display.println(J3_name);
              display.println(" "+J1orJ2_Ant);
              display.display(); 
              //END DISPLAY  
            }

//==================================================================J4 Antenna selection
            if (header.indexOf("GET /3/off") >= 0) {
              Serial.println("Turning on " +J4_name);
        

                J4_Ant = "on";
                J3_Ant = "off";
                J6_Ant = "off";
                J5_Ant= "off";
                J1orJ2_Ant = "Remote";
              digitalWrite(Output15, LOW);
              digitalWrite(Output13, LOW);//K1
              digitalWrite(Output0, LOW); //K2
              digitalWrite(Output12, HIGH);//K3
              //Display 
              display.clearDisplay();
              display.setTextSize(2);
              display.setCursor(0, 27);
              display.println(J4_name);
              display.println(" "+J1orJ2_Ant);
           
              

              display.display(); 
              //END DISPLAY  
            }




//==================================================================J1 or J2 Antenna selection
            if (header.indexOf("GET /4/on") >= 0) {
              Serial.println("Switching to " +J2_name);
        

              J1orJ2_Ant = "Local";
              J4_Ant = "off";
              J3_Ant = "off";
              J6_Ant = "off";
              J5_Ant= "off";
              digitalWrite(Output15, HIGH);

                //Display 
              display.clearDisplay();
              display.setTextSize(2);
              display.setCursor(0, 27);
              display.println("");
              display.println(" "+J1orJ2_Ant);
              display.display(); 
        
              
            }

 //==============================================================================================================                    
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            //client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println(".button { background-color: brown; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            // client.println(".button2 {background-color: #77878A;}</style></head>");
            client.println(".button2 {background-color: blue;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1> AWA Remote Coax Switch "+(String)SW_VER+"</h1>");
//==============================================================================================================            
            // Display current state, for J5 Connector  
            client.println("<p> " +J5_name + " " /*+ J5_Ant*/ + "</p>");
            // If the J5_Antis off, it displays the ON button       
            if (J5_Ant=="on") {
              client.println("<p><a href=\"/0/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/0/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
//==============================================================================================================               
            // Display current state, J6 Connector 
            client.println("<p> " +J6_name + " " /*+ J6_Ant*/ + "</p>");
            // If the J6_Ant is off, it displays the ON button       
            if (J6_Ant=="on") {
              client.println("<p><a href=\"/1/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/1/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("</body></html>");
//==============================================================================================================
            // Display current state, for J3 Connector 
            client.println("<p> "+J3_name + " " /*+ J3_Ant*/ + "</p>");
            // If the J3_Ant is off, it displays the ON button       
            if (J3_Ant=="on") {
              client.println("<p><a href=\"/2/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/2/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("</body></html>");
//==============================================================================================================
            // Display current state, for J4 Connector
            client.println("<p> " +J4_name +" " /*+ J4_Ant*/ + "</p>");
            // If the J4_Ant is off, it displays the ON button       
            if (J4_Ant=="on") {
              client.println("<p><a href=\"/3/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/3/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("</body></html>");
//==============================================================================================================
             // Display current state, of J1/J2 connectors 
            client.println("<p>" +J1_name +" or " +J2_name /*+ " set to " + J1orJ2_Ant*/ + "</p>");
            // If the Remote or Local is off, it displays the Remote button       
            if (J1orJ2_Ant=="Remote") {
              client.println("<p><a href=\"/4/on\"><button class=\"button\">Remote</button></a></p>");
            } else {
              client.println("<p><a href=\"/4/off\"><button class=\"button\">LOCAL ANTENNA</button></a></p>");
            }
            client.println("</body></html>");

//==============================================================================================================           
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
      } // if (client.available())
    }  // while (client.connected...)
    
    // Clear the header variable
    header = "";

    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }  // if (client)

}  // clientUpdate()

//==============================================================================================================                    
  void clientReboot(){

  client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;

    //==============================================================================================================                    
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis(); 
   

      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
//==================================================================J5 Antenna Selection selection
            if ((header.indexOf("GET /4/off") >= 0) ||
                (header.indexOf("GET /4/on") >= 0)) 
                {
                  Serial.println("Rebooting....");
                  if (rebootFlag == 1) ESP.restart();
                }
 //==============================================================================================================                    
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            //client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println(".button { background-color: brown; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            // client.println(".button2 {background-color: #77878A;}</style></head>");
            client.println(".button2 {background-color: blue;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1> AWA Remote Coax Switch "+(String)SW_VER+"</h1>");
            client.println("<body><h1> Physical Button Mode</h1>");
//==============================================================================================================            
            client.println("<p><a href=\"/4/on\"><button class=\"button\">REBOOT</button></a></p>");
//==============================================================================================================           
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
      } // if (client.available())
    }  // while (client.connected...)
    
    // Clear the header variable
    header = "";
    rebootFlag = 1;

    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }  // if (client)

}  // clientReboot()

//==============================================================================================================                    
void physButtonPress() 
{
   
    pbState = digitalRead(Input14);
    //Serial.println(pbState);

      if (pbState < prevPbState)  // phsical button press detected
      {
        prevPbState  = 1;
        if (J5_Ant == "on") {  // J5->J6
          Serial.println("physButtonPress: Switching to " +J6_name);

          J6_Ant = "on";
          J5_Ant= "off";
          J3_Ant = "off";
          J4_Ant = "off";
          J1orJ2_Ant = "Remote";

          digitalWrite(Output13, HIGH); //K1
          digitalWrite(Output0, LOW); 
          digitalWrite(Output12, LOW);
          digitalWrite(Output15, LOW);
          
          //Display 
          display.clearDisplay();
          display.setTextSize(2);
          display.setCursor(0, 27);
          display.println(J6_name);
          //display.println(" "+J6_Ant);
          display.display(); 
          //END DISPLAY

        }
        else if (J6_Ant == "on") // J6->J3
        {
+         Serial.println("physButtonPress: Switching to " +J3_name);

          J3_Ant = "on";
          J6_Ant = "off";
          J5_Ant= "off";
          J4_Ant = "off";
          J1orJ2_Ant = "Remote";
            
          digitalWrite(Output13, LOW);//K1
          digitalWrite(Output0, HIGH); //K2
          digitalWrite(Output12, HIGH);//K3
          digitalWrite(Output15, LOW);

          //Display 
          display.clearDisplay();
          display.setTextSize(2);
          display.setCursor(0, 27);
          display.println(J3_name);
          //display.println(" "+J3_Ant);
          display.display(); 
          //END DISPLAY  

        }
        else if (J3_Ant == "on")   // J3->J4
        {
          Serial.println("physButtonPress: Switching to " +J4_name);
    
          J4_Ant = "on";
          J3_Ant = "off";
          J6_Ant = "off";
          J5_Ant= "off";
          J1orJ2_Ant = "Remote";

          digitalWrite(Output13, LOW);//K1
          digitalWrite(Output0, LOW); //K2
          digitalWrite(Output12, HIGH);//K3
          digitalWrite(Output15, LOW);

          //Display 
          display.clearDisplay();
          display.setTextSize(2);
          display.setCursor(0, 27);
          display.println(J4_name);
          //display.println(" "+J4_Ant);
          display.display(); 
          //END DISPLAY  

        }

        else if (J4_Ant == "on")  // J4->J2
        {
          Serial.println("physButtonPress: Switching to " + J2_name);
    
          J1orJ2_Ant = "Local";
          J6_Ant = "off";
          J5_Ant= "off";
          J3_Ant = "off";
          J4_Ant = "off";
          
          digitalWrite(Output13, LOW);
          digitalWrite(Output0, LOW); 
          digitalWrite(Output12, LOW); 
          digitalWrite(Output15, HIGH);
          
            //Display 
          display.clearDisplay();
          display.setTextSize(2);
          display.setCursor(0, 27);
          display.println(" "+J1orJ2_Ant);
          display.display();
          
        }
        else if (J1orJ2_Ant == "Local") // Local->J5
        {
          Serial.println("physButtonPress: Switching to "+J5_name);

          J5_Ant= "on";
          J6_Ant = "off";
          J3_Ant = "off";
          J4_Ant = "off";
          J1orJ2_Ant = "Remote";

          digitalWrite(Output13, LOW);
          digitalWrite(Output0, LOW); 
          digitalWrite(Output12, LOW);
          digitalWrite(Output15, LOW);

          //Display 
          display.clearDisplay();
          display.setTextSize(2);
          display.setCursor(0, 27);
          display.println(J5_name);
          //display.println(" "+J5_Ant);
          display.display(); 
          //END DISPLAY  
          
        } 

        // debounce
        delay(1000);

      } // physical button press     
}  // physButtonPress()

//==============================================================================================================                    
void loop(){

  if (physButtonMode){
    physButtonPress();
    clientReboot();
  } 
  else{    
    clientUpdate();
  }
  
}  // main loop
