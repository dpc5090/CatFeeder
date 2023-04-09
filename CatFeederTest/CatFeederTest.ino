
#include <SPI.h>
#include <WiFiNINA.h>
#include <Servo.h>
#include <RFID.h>

#include "config.h"
#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
#define SS_PIN 10
#define RST_PIN 9
#define CW 7
RFID rfid(SS_PIN, RST_PIN);

String rfidCard;
Servo door;
Servo auger;
String validCard = "145 84 101 29";
bool isDoorOpen = false;
long int lastTimeCatSeen = 0;
int setupHour;
int setupMinute;
int curDay;
bool amFoodDone = false;
bool pmFoodDone = false;
int a_time = AUGING_TIME_MS;
String logs[10];
void logMessage(String message){
  for (int i = 8; i>=0; i -=1)
  {
    logs[i+1] = logs[i];
  }
  logs[0] = message;
}
void openDoor(){
  Serial.println("Opening Door");
  door.write(0);
  delay(DOOR_CLOSE_MS);
  door.write(90);
  isDoorOpen = true;
  lastTimeCatSeen = millis();
  logMessage("OPEN DOOR");

}
void closeDoor(){
  Serial.println("Closing Door");
  door.write(180);
  delay(DOOR_CLOSE_MS);
  door.write(90);
  isDoorOpen = false;
  logMessage("CLOSE DOOR");


}
void runAuger()
{
  Serial.println("Drill BB Drill");
  auger.write(0);
  delay(a_time);
  auger.write(90);
  logMessage("RAN AUGER");
  Serial.println("Done son");
}
int status = WL_IDLE_STATUS;
WiFiServer server(80);
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}
void setup() {
  Serial.begin(9600);      // initialize serial communication
  SPI.begin();
  rfid.init();
  door.attach(8);
  auger.attach(7);
  pinMode(CW, OUTPUT);
  
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);

  }
  server.begin();                           // start the web server on port 80
  printWifiStatus();  
  int tod =(WiFi.getTime()-(3600*5))%86400;  
  int day = (WiFi.getTime()-(3600*5))/86400;
  int hour =      tod/3600;
  int minute = (tod%3600)/60;           // you're connected now, so print out the status

  curDay = day;
  setupHour = hour;
  setupMinute = minute;
  if (setupHour >=FEED_TIME_AM ){
    amFoodDone = true;
  }
  if (setupHour >=FEED_TIME_PM ){
    pmFoodDone = true;
  }
  Serial.println(amFoodDone);

  Serial.println(pmFoodDone);  

  for (int i=0; i<10;i+=1)
  {
    logs[i] = "";
  }
}
void rfidRoutine(){
  if (rfid.isCard()){
    if (rfid.readCardSerial()) {

      rfidCard = String(rfid.serNum[0]) + " " + String(rfid.serNum[1]) + " " + String(rfid.serNum[2]) + " " + String(rfid.serNum[3]);
      Serial.println(rfidCard);
      if(rfidCard == validCard){
        if (isDoorOpen == false)
        {
          openDoor();
          Serial.println("COLONEL IS HERE BB");
          logMessage("COLONEL ARRIVED");
        }
        lastTimeCatSeen = millis();
        
      }
      else{
        if (isDoorOpen == true){
          Serial.println("Get outta here Tanis!");
          logMessage("BEGONE");
          closeDoor();
        }
      }
    }
  }
  else{
    if (isDoorOpen == true){
      if (millis() - lastTimeCatSeen> 10000){
        Serial.println("Aint nobody home");
        closeDoor();
      }
    }
  
  }
}
void checkFeed()
{
  int tod =(WiFi.getTime()-(3600*5))%86400;  
  int hour =      tod/3600;
  if(amFoodDone == false)
  {
    if(hour >= FEED_TIME_AM)
    {
      amFoodDone = true;
      Serial.println("MORNING FOOD TIME");
      runAuger();

    }
  }
  if(pmFoodDone == false)
  {
    if(hour >= FEED_TIME_PM)
    {
      pmFoodDone = true;
      Serial.println("EVENING FOOD TIME");
      runAuger();
    }
  }
}
void checkDay()
{
  int day = (WiFi.getTime()-(3600*5))/86400;
  if( day> curDay){
    pmFoodDone = false;
    amFoodDone = false;
  }
}
void displayPage(WiFiClient client)
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();

  // the content of the HTTP response follows the header:
  client.print("Morning Food time:");
  client.print(FEED_TIME_AM);
  client.print("<br>");
  client.print("Evening Food time:");
  client.print(FEED_TIME_PM);
  client.print("<br><br><br>");
  client.print("<form action=\"/action_page.php\">");
  client.print("<label for=\"fname\">Auging Time (MS): </label><input type=\"text\" id=\"atime\" name=\"atime\" value=\""+String(a_time)+"\"><br><br><input type=\"submit\" value=\"Update Config\"></form>");
  
  client.print("<button onclick=\"window.location.href=\'/O\';\">\n Open Door\n</button><br><br>\n");
  client.print("<button onclick=\"window.location.href=\'/C\';\">\n Close Door\n</button><br><br>\n");
  client.print("<button onclick=\"window.location.href=\'/A\';\">\n Run Auger\n</button><br><br><br>\n");
  client.print("<button onclick=\"window.location.href=\'/L\';\">\n Update Log\n</button><br><br><br>\n");
  for (int i = 0; i<10; i+=1){
    client.print(logs[i]);
    client.print("<br>");
  }

  // The HTTP response ends with another blank line:
  client.println();
}
void handleMessage(String m)
{
  Serial.println(m);

  if (m.startsWith("GET /A")) {
    runAuger();         
  }
  if (m.startsWith("GET /O")) {
    openDoor();
  }
  if (m.startsWith("GET /C")) {
    closeDoor();   
  }
  if (m.startsWith("GET /L")) {
  }
  if (m.startsWith("GET /action_page.php?atime=")) {
    int i = m.indexOf("=")+1;
    String s_str = m.substring(i);
    i = s_str.indexOf(" ");
    s_str = s_str.substring(0,i);
    a_time = s_str.toInt();
    Serial.println(s_str);
    logMessage("AUGING TIME RESET To: "+s_str);
  }
}
void loop() {
  checkDay();
  checkFeed();
  rfidRoutine();
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        // Serial.write(c);    
                        // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            displayPage(client);
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            // Serial.println(currentLine);
            handleMessage(currentLine);
            // displayPage(client);
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        
      }
      
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");  
  }
  
}


