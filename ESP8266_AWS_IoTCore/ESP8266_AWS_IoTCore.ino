
//Temperature
#include <DHT.h>
#include <DHT_U.h>
#include <Hash.h>
#include <Adafruit_Sensor.h>


//Gyroscope
#include <Wire.h>

//Air Quality Sensor
#include <MQ135.h>

#include <PubSubClient.h>

#include <FS.h>
#include <NTPClient.h>

#include <ESP8266WebServer.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiGratuitous.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiServer.h>
#include <WiFiServerSecure.h>
#include <WiFiServerSecureBearSSL.h>
#include <WiFiUdp.h>

String apiKey = "R2M4QUP18HXS0Q9P";     //  Enter your Write API key from ThingSpeak

int slno = 0;
const char *ssid =  "Swayam OnePlus 7T";     // replace with your wifi ssid and wpa2 key
const char *password =  "Sam12345";
const char* things_server = "api.thingspeak.com";

//Temperature and Humidity Sensor
const int dht_pin = 16;
#define DHTTYPE DHT11   // DHT 11
// Initialize DHT sensor.
DHT dht(dht_pin, DHTTYPE);
float temp;
float humid;

//Gyroscope
const int MPU_addr = 0x68; // I2C address of the MPU-6050
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;
float ax = 0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0;
boolean fall = false; //stores if a fall has occurred
boolean trigger1 = false; //stores if first trigger (lower threshold) has occurred
boolean trigger2 = false; //stores if second trigger (upper threshold) has occurred
boolean trigger3 = false; //stores if third trigger (orientation change) has occurred
byte trigger1count = 0; //stores the counts past since trigger 1 was set true
byte trigger2count = 0; //stores the counts past since trigger 2 was set true
byte trigger3count = 0; //stores the counts past since trigger 3 was set true
int angleChange = 0;
int Amp = 0;

//Ultrasonic Sensor
// defines pins numbers
const int trigPin = D5;
const int echoPin = D6;
// defines variables
long duration;
int distance;

//IR
const int ir_pin = D7;
int ir_val = 0;

//PIR
const int pir_pin = D8;
long pir_val = 0;

//Air Quality Sensor
const int air_pin = A0; //z_pin
float air_quality = 0;      //z_acc

ESP8266WebServer server(80);
WiFiClient things_client;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
const char* AWS_endpoint = "a2ktgdtuibpw91-ats.iot.us-east-1.amazonaws.com"; //MQTT broker ip
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
WiFiClientSecure espClient;
PubSubClient client(AWS_endpoint, 8883, callback, espClient); //set MQTT port number to 8883 as per //standard
long lastMsg = 0;
char msg[512];
int value = 5324;


void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  espClient.setBufferSizes(512, 512);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  timeClient.begin();
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  espClient.setX509Time(timeClient.getEpochTime());
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESPthing")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      char buf[256];
      espClient.getLastSSLError(buf, 256);
      Serial.print("WiFiClientSecure SSL error: ");
      Serial.println(buf);
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin();
  Serial.setDebugOutput(true);
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  //Temperature and Humidity Sensor
  pinMode(dht_pin, INPUT);

  //Gyroscope
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  Serial.println("Wrote to IMU");

  //Ultrasonic
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  //IR
  pinMode(ir_pin, INPUT);

  //PIR
  pinMode(pir_pin, INPUT);

  setup_wifi();
  delay(1000);
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());
  // Load certificate file
  File cert = SPIFFS.open("/cert.der", "r"); //replace cert.crt eith your uploaded file name
  if (!cert) {
    Serial.println("Failed to open cert file");
  }
  else
    Serial.println("Success to open cert file");
  delay(1000);
  if (espClient.loadCertificate(cert))
    Serial.println("cert loaded");
  else
    Serial.println("cert not loaded");
  // Load private key file
  File private_key = SPIFFS.open("/private.der", "r"); //replace private eith your uploaded file name
  if (!private_key) {
    Serial.println("Failed to open private cert file");
  }
  else
    Serial.println("Success to open private cert file");
  delay(1000);
  if (espClient.loadPrivateKey(private_key))
    Serial.println("private key loaded");
  else
    Serial.println("private key not loaded");
  // Load CA file
  File ca = SPIFFS.open("/ca.der", "r"); //replace ca eith your uploaded file name
  if (!ca) {
    Serial.println("Failed to open ca ");
  }
  else
    Serial.println("Success to open ca");
  delay(1000);
  if (espClient.loadCACert(ca))
    Serial.println("ca loaded");
  else
    Serial.println("ca failed");
  Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());

  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");

}
void loop() {
  slno++;
  if(fall==1){
    fall=0;
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  long now = millis();

  //Temperature and Humidity Sensor
  float newT = dht.readTemperature();
  if(!(isnan(newT))){
    temp = newT;
  }
  float newH = dht.readHumidity();
  if(!(isnan(newH))){
    humid = newT;
  }
//  temp = dht.readTemperature(); // Gets the values of the temperature
//  humid = dht.readHumidity(); // Gets the values of the humidity

  //Ultrasonic
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance = duration * 0.034 / 2;

  //Gyroscope
  mpu_read();
  ax = (AcX - 2050 - 0.88*16384) / 16384.00;
  ay = (AcY - 77 - 0.02*16384) / 16384.00;
  az = (AcZ - 1947 + 0.05*16384) / 16384.00;
//  Serial.print(ax);
//  Serial.print("\t");
//  Serial.print(ay);
//  Serial.print("\t");
//  Serial.print(az);
//  Serial.print("\t");
  gx = (GyX + 270) / 131.07;
  gy = (GyY - 351) / 131.07;
  gz = (GyZ + 136) / 131.07;
  // calculating Amplitute vector for 3 axis
  float Raw_Amp = pow(pow(ax, 2) + pow(ay, 2) + pow(az, 2), 0.5);
  Amp = Raw_Amp * 10 - 1;  // Mulitiplied by 10 bcz values are between 0 to 1
//  Amp = 0;
  if(Amp >= 3){
    fall = 1;
  }
//  if (Amp <= 2 && trigger2 == false) { //if AM breaks lower threshold (0.4g)
//    trigger1 = true;
////    Serial.println("TRIGGER 1 ACTIVATED");
//  }
//  if (trigger1 == true) {
//    trigger1count++;
//    if (Amp >= 12) { //if AM breaks upper threshold (3g)
//      trigger2 = true;
////      Serial.println("TRIGGER 2 ACTIVATED");
//      trigger1 = false; trigger1count = 0;
//    }
//  }
//  if (trigger2 == true) {
//    trigger2count++;
//    angleChange = pow(pow(gx, 2) + pow(gy, 2) + pow(gz, 2), 0.5); Serial.println(angleChange);
//    if (angleChange >= 30 && angleChange <= 400) { //if orientation changes by between 80-100 degrees
//      trigger3 = true; trigger2 = false; trigger2count = 0;
////      Serial.println(angleChange);
////      Serial.println("TRIGGER 3 ACTIVATED");
//    }
//  }
//  if (trigger3 == true) {
//    trigger3count++;
//    if (trigger3count >= 10) {
//      angleChange = pow(pow(gx, 2) + pow(gy, 2) + pow(gz, 2), 0.5);
//      //delay(10);
////      Serial.println(angleChange);
//      if ((angleChange >= 0) && (angleChange <= 10)) { //if orientation changes remains between 0-10 degrees
//        fall = true; trigger3 = false; trigger3count = 0;
////        Serial.println(angleChange);
//      }
//      else { //user regained normal orientation
//        trigger3 = false; trigger3count = 0;
////        Serial.println("TRIGGER 3 DEACTIVATED");
//      }
//    }
//  }
////  if (fall == true) { //in event of a fall detection
////    Serial.println("FALL DETECTED");
////    send_event("fall_detect");
////    fall = false;
////  }
//  if (trigger2count >= 6) { //allow 0.5s for orientation change
//    trigger2 = false; trigger2count = 0;
////    Serial.println("TRIGGER 2 DECACTIVATED");
//  }
//  if (trigger1count >= 6) { //allow 0.5s for AM to break upper threshold
//    trigger1 = false; trigger1count = 0;
////    Serial.println("TRIGGER 1 DECACTIVATED");
//  }
//  if(fall == 1){
//    digitalWrite(D3, HIGH);
//  }
 
  //IR
  ir_val = digitalRead(ir_pin);
  if (ir_val == 0) ir_val = 1;
  else ir_val = 0;

  //PIR
  pir_val = digitalRead(pir_pin);

  //Air Quality Sensor
  MQ135 gasSensor = MQ135(air_pin);
  air_quality = gasSensor.getPPM()/1024*100;

  server.handleClient();

  if (things_client.connect(things_server, 80))  //   "184.106.153.149" or api.thingspeak.com
  {

    String postStr = apiKey;
    postStr += "&field1=";
    postStr += String(temp);
    postStr += "&field2=";
    postStr += String(humid);
    postStr += "&field3=";
    postStr += String(Amp);
    postStr += "&field4=";
    postStr += String(fall);
    postStr += "&field5=";
    postStr += String(distance);
    postStr += "&field6=";
    postStr += String(ir_val);
    postStr += "&field7=";
    postStr += String(pir_val);
    postStr += "&field8=";
    postStr += String(air_quality);
    postStr += "\r\n\r\n";

    things_client.print("POST /update HTTP/1.1\n");
    things_client.print("Host: api.thingspeak.com\n");
    things_client.print("Connection: close\n");
    things_client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    things_client.print("Content-Type: application/x-www-form-urlencoded\n");
    things_client.print("Content-Length: ");
    things_client.print(postStr.length());
    things_client.print("\n\n");
    things_client.print(postStr);

    Serial.print("\nTemperature: ");
    Serial.print(temp);
    Serial.print("\tHumidity: ");
    Serial.print(humid);
    Serial.print("\nAcceleration: ");
    Serial.print(Amp);
    Serial.print("\tFall: ");
    Serial.print(fall);
    Serial.print("\nDistance: ");
    Serial.print(distance);
    //    Serial.print("\nZ Acceleration: ");
    //    Serial.print(air);
    Serial.print("\nObject Detected: ");
    Serial.print(ir_val);
    Serial.print("\tMotion Detected: ");
    Serial.print(pir_val);
    Serial.print("\nAir Quality: ");
    Serial.print(air_quality);
    Serial.println("\n\nSend to Thingspeak.");
  }
  things_client.stop();

  Serial.println("Waiting...\n");

  // thingspeak needs minimum 15 sec delay between updates
  delay(1000);

  if (now - lastMsg > 2000) {

    // Prints the distance on the Serial Monitor
    lastMsg = now;
    String s2 = "LA";
    String falls;
    if(fall == 1){
      falls = "True";
    }
    else{
      falls = "False";
    }
    String objects;
    if(ir_val > 0){
      objects = "True";
    }
    else{
      objects = "False";
    }
    String motions;
    if(pir_val > 0){
      motions = "True";
    }
    else{
      motions = "False";
    }
    snprintf (msg, 512, "{\"Sl_No\": %d, \"Temperature\": %f, \"Humidity\": %f, \"Acceleration\": %d, \"Fall\": \"%s\", \"Distance\": %d, \"Object Detected\": \"%s\", \"Motion Detected\": \"%s\", \"Air Quality\": %f}", slno, temp, humid, Amp, falls.c_str(), distance, objects.c_str(), motions.c_str(), air_quality);
//        snprintf (msg, 75, "{\"mac_Id\" : \"%s\", \"ts\" : %d, \"Distance\" : %d, \"Rahul\" : \"%s\", \"ZAcceleration\" : %d}", "macabc".c_str(), distance, distance,"LA".c_str(),69);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("outTopic", msg);
//    Serial.print("Heap: "); Serial.println(ESP.getFreeHeap()); //Low heap can cause problems
  }
  digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
  delay(100); // wait for a second
  digitalWrite(LED_BUILTIN, LOW); // turn the LED off by making the voltage LOW
  delay(100); // wait for a second
}

void mpu_read(){ //Gyroscope
 Wire.beginTransmission(MPU_addr);
 Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
 Wire.endTransmission(false);
 Wire.requestFrom(MPU_addr,14,true);  // request a total of 14 registers
 AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
 AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
 AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
 Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
 GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
 GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
 GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
}

void handle_OnConnect() {

  server.send(200, "text/html", SendHTML(temp, humid, Amp, fall, distance, ir_val, pir_val, air_quality));

}

void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

String SendHTML(float temp, float humid, int Amp, boolean fall, int distance, int ir_val, long pir_val, float air_quality) {
  String ptr = "<!DOCTYPE html>";
  ptr += "<html>";
  ptr += "<head>";
  ptr += "<title>ESP32 Patient Health Monitoring</title>";
  ptr += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  ptr += "<link href='https://fonts.googleapis.com/css?family=Open+Sans:300,400,600' rel='stylesheet'>";
  ptr += "<link href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css' rel='stylesheet'>";
  ptr += "<style>";
  ptr += "html { font-family: 'Open Sans', sans-serif; display: block; margin: 0px auto; text-align: center;color: #444444;}";
  ptr += "body{margin: 0px;} ";
  ptr += "h1 {margin: 50px auto 30px;} ";
  ptr += ".side-by-side{display: table-cell;vertical-align: middle;position: relative;}";
  ptr += ".text{font-weight: 600;font-size: 19px;width: 200px;text-align: left;margin: 0px 8px}";
  ptr += ".reading{font-weight: 300;font-size: 50px;padding-right: 25px;}";

  ptr += ".tem .reading{color: #FF0000;}";
  ptr += ".hum .reading{color: #0000FF;}";
  ptr += ".dist .reading{color: #F29C1F;}";
  ptr += ".acc .reading{color: #00FF00;}";
  ptr += ".fal .reading{color: #FF0000;}";
  ptr += ".irval .reading{color: #FF0000;}";
  ptr += ".pirval .reading{color: #955BA5;}";
  ptr += ".airq .reading{color: #777777;}";
  /*ptr +=".bodytemperature .reading{color: #F29C1F;}";*/

  ptr += ".superscript{font-size: 17px;font-weight: 600;position: absolute;top: 10px;}";
  ptr += ".data{padding: 10px;}";
  ptr += ".container{display: table;margin: 0 auto;}";
  ptr += ".icon{width:65px}";
  ptr += "</style>";
  ptr += "</head>";
  ptr += "<body>";
  ptr += "<h1>ESP8266 Patient Health Monitoring</h1>";
  ptr += "<h3>Healthify_ESP8266</h3>";
  ptr += "<div class='container'>";

   ptr += "<div class='data dist'>";
//  ptr += "<div class='side-by-side icon'>";
//  ptr += "<svg enable-background='new 0 0 19.438 54.003'height=54.003px id=Layer_1 version=1.1 viewBox='0 0 19.438 54.003'width=19.438px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M11.976,8.82v-2h4.084V6.063C16.06,2.715,13.345,0,9.996,0H9.313C5.965,0,3.252,2.715,3.252,6.063v30.982";
//  ptr += "C1.261,38.825,0,41.403,0,44.286c0,5.367,4.351,9.718,9.719,9.718c5.368,0,9.719-4.351,9.719-9.718";
//  ptr += "c0-2.943-1.312-5.574-3.378-7.355V18.436h-3.914v-2h3.914v-2.808h-4.084v-2h4.084V8.82H11.976z M15.302,44.833";
//  ptr += "c0,3.083-2.5,5.583-5.583,5.583s-5.583-2.5-5.583-5.583c0-2.279,1.368-4.236,3.326-5.104V24.257C7.462,23.01,8.472,22,9.719,22";
//  ptr += "s2.257,1.01,2.257,2.257V39.73C13.934,40.597,15.302,42.554,15.302,44.833z'fill=#F29C21 /></g></svg>";
//  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Distance</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (int)distance;
  ptr += "<span>cm</span></div>";
  ptr += "</div>";

  ptr += "<div class='data acc'>";
//  ptr += "<div class='side-by-side icon'>";
//  ptr += "<svg enable-background='new 0 0 19.438 54.003'height=54.003px id=Layer_1 version=1.1 viewBox='0 0 19.438 54.003'width=19.438px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M11.976,8.82v-2h4.084V6.063C16.06,2.715,13.345,0,9.996,0H9.313C5.965,0,3.252,2.715,3.252,6.063v30.982";
//  ptr += "C1.261,38.825,0,41.403,0,44.286c0,5.367,4.351,9.718,9.719,9.718c5.368,0,9.719-4.351,9.719-9.718";
//  ptr += "c0-2.943-1.312-5.574-3.378-7.355V18.436h-3.914v-2h3.914v-2.808h-4.084v-2h4.084V8.82H11.976z M15.302,44.833";
//  ptr += "c0,3.083-2.5,5.583-5.583,5.583s-5.583-2.5-5.583-5.583c0-2.279,1.368-4.236,3.326-5.104V24.257C7.462,23.01,8.472,22,9.719,22";
//  ptr += "s2.257,1.01,2.257,2.257V39.73C13.934,40.597,15.302,42.554,15.302,44.833z'fill=#F29C21 /></g></svg>";
//  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Acceleration Amplitude</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (int)Amp;
  ptr += "<span>m/s<span class='superscript'>2</span></span></div>";
  ptr += "</div>";

      ptr += "<div class='data fal'>";
//  ptr += "<div class='side-by-side icon'>";
//  ptr += "<svg enable-background='new 0 0 19.438 54.003'height=54.003px id=Layer_1 version=1.1 viewBox='0 0 19.438 54.003'width=19.438px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M11.976,8.82v-2h4.084V6.063C16.06,2.715,13.345,0,9.996,0H9.313C5.965,0,3.252,2.715,3.252,6.063v30.982";
//  ptr += "C1.261,38.825,0,41.403,0,44.286c0,5.367,4.351,9.718,9.719,9.718c5.368,0,9.719-4.351,9.719-9.718";
//  ptr += "c0-2.943-1.312-5.574-3.378-7.355V18.436h-3.914v-2h3.914v-2.808h-4.084v-2h4.084V8.82H11.976z M15.302,44.833";
//  ptr += "c0,3.083-2.5,5.583-5.583,5.583s-5.583-2.5-5.583-5.583c0-2.279,1.368-4.236,3.326-5.104V24.257C7.462,23.01,8.472,22,9.719,22";
//  ptr += "s2.257,1.01,2.257,2.257V39.73C13.934,40.597,15.302,42.554,15.302,44.833z'fill=#F29C21 /></g></svg>";
//  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Fall</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (int)fall;
  /*ptr += "<span>cm</span></div>";*/
  ptr += "</div>";
  ptr += "</div>";

  ptr += "<div class='data irval'>";
//  ptr += "<div class='side-by-side icon'>";
//  ptr += "<svg enable-background='new 0 0 40.542 40.541'height=40.541px id=Layer_1 version=1.1 viewBox='0 0 40.542 40.541'width=40.542px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M34.313,20.271c0-0.552,0.447-1,1-1h5.178c-0.236-4.841-2.163-9.228-5.214-12.593l-3.425,3.424";
//  ptr += "c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293c-0.391-0.391-0.391-1.023,0-1.414l3.425-3.424";
//  ptr += "c-3.375-3.059-7.776-4.987-12.634-5.215c0.015,0.067,0.041,0.13,0.041,0.202v4.687c0,0.552-0.447,1-1,1s-1-0.448-1-1V0.25";
//  ptr += "c0-0.071,0.026-0.134,0.041-0.202C14.39,0.279,9.936,2.256,6.544,5.385l3.576,3.577c0.391,0.391,0.391,1.024,0,1.414";
//  ptr += "c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293L5.142,6.812c-2.98,3.348-4.858,7.682-5.092,12.459h4.804";
//  ptr += "c0.552,0,1,0.448,1,1s-0.448,1-1,1H0.05c0.525,10.728,9.362,19.271,20.22,19.271c10.857,0,19.696-8.543,20.22-19.271h-5.178";
//  ptr += "C34.76,21.271,34.313,20.823,34.313,20.271z M23.084,22.037c-0.559,1.561-2.274,2.372-3.833,1.814";
//  ptr += "c-1.561-0.557-2.373-2.272-1.815-3.833c0.372-1.041,1.263-1.737,2.277-1.928L25.2,7.202L22.497,19.05";
//  ptr += "C23.196,19.843,23.464,20.973,23.084,22.037z'fill=#26B999 /></g></svg>";
//  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Object Detected</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (int)ir_val;
  /*ptr +="<span class='superscript'>BPM</span></div>";*/
  ptr += "</div>";
  ptr += "</div>";

  ptr += "<div class='data pirval'>";
//  ptr += "<div class='side-by-side icon'>";
//  ptr += "<svg enable-background='new 0 0 58.422 40.639'height=40.639px id=Layer_1 version=1.1 viewBox='0 0 58.422 40.639'width=58.422px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M58.203,37.754l0.007-0.004L42.09,9.935l-0.001,0.001c-0.356-0.543-0.969-0.902-1.667-0.902";
//  ptr += "c-0.655,0-1.231,0.32-1.595,0.808l-0.011-0.007l-0.039,0.067c-0.021,0.03-0.035,0.063-0.054,0.094L22.78,37.692l0.008,0.004";
//  ptr += "c-0.149,0.28-0.242,0.594-0.242,0.934c0,1.102,0.894,1.995,1.994,1.995v0.015h31.888c1.101,0,1.994-0.893,1.994-1.994";
//  ptr += "C58.422,38.323,58.339,38.024,58.203,37.754z'fill=#955BA5 /><path d='M19.704,38.674l-0.013-0.004l13.544-23.522L25.13,1.156l-0.002,0.001C24.671,0.459,23.885,0,22.985,0";
//  ptr += "c-0.84,0-1.582,0.41-2.051,1.038l-0.016-0.01L20.87,1.114c-0.025,0.039-0.046,0.082-0.068,0.124L0.299,36.851l0.013,0.004";
//  ptr += "C0.117,37.215,0,37.62,0,38.059c0,1.412,1.147,2.565,2.565,2.565v0.015h16.989c-0.091-0.256-0.149-0.526-0.149-0.813";
//  ptr += "C19.405,39.407,19.518,39.019,19.704,38.674z'fill=#955BA5 /></g></svg>";
//  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Motion Detected</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (int)pir_val;
  /*ptr +="<span class='superscript'>%</span></div>";*/
  ptr += "</div>";
  ptr += "</div>";

      ptr += "<div class='data airq'>";
//  ptr += "<div class='side-by-side icon'>";
//  ptr += "<svg enable-background='new 0 0 58.422 40.639'height=40.639px id=Layer_1 version=1.1 viewBox='0 0 58.422 40.639'width=58.422px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M58.203,37.754l0.007-0.004L42.09,9.935l-0.001,0.001c-0.356-0.543-0.969-0.902-1.667-0.902";
//  ptr += "c-0.655,0-1.231,0.32-1.595,0.808l-0.011-0.007l-0.039,0.067c-0.021,0.03-0.035,0.063-0.054,0.094L22.78,37.692l0.008,0.004";
//  ptr += "c-0.149,0.28-0.242,0.594-0.242,0.934c0,1.102,0.894,1.995,1.994,1.995v0.015h31.888c1.101,0,1.994-0.893,1.994-1.994";
//  ptr += "C58.422,38.323,58.339,38.024,58.203,37.754z'fill=#955BA5 /><path d='M19.704,38.674l-0.013-0.004l13.544-23.522L25.13,1.156l-0.002,0.001C24.671,0.459,23.885,0,22.985,0";
//  ptr += "c-0.84,0-1.582,0.41-2.051,1.038l-0.016-0.01L20.87,1.114c-0.025,0.039-0.046,0.082-0.068,0.124L0.299,36.851l0.013,0.004";
//  ptr += "C0.117,37.215,0,37.62,0,38.059c0,1.412,1.147,2.565,2.565,2.565v0.015h16.989c-0.091-0.256-0.149-0.526-0.149-0.813";
//  ptr += "C19.405,39.407,19.518,39.019,19.704,38.674z'fill=#955BA5 /></g></svg>";
//  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Air Quality</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (int)air_quality;
  ptr += "<span>ppm</span></div>";
  ptr += "</div>";

  ptr += "<div class='data tem'>";
//  ptr += "<div class='side-by-side icon'>";
//  ptr += "<svg enable-background='new 0 0 19.438 54.003'height=54.003px id=Layer_1 version=1.1 viewBox='0 0 19.438 54.003'width=19.438px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M11.976,8.82v-2h4.084V6.063C16.06,2.715,13.345,0,9.996,0H9.313C5.965,0,3.252,2.715,3.252,6.063v30.982";
//  ptr += "C1.261,38.825,0,41.403,0,44.286c0,5.367,4.351,9.718,9.719,9.718c5.368,0,9.719-4.351,9.719-9.718";
//  ptr += "c0-2.943-1.312-5.574-3.378-7.355V18.436h-3.914v-2h3.914v-2.808h-4.084v-2h4.084V8.82H11.976z M15.302,44.833";
//  ptr += "c0,3.083-2.5,5.583-5.583,5.583s-5.583-2.5-5.583-5.583c0-2.279,1.368-4.236,3.326-5.104V24.257C7.462,23.01,8.472,22,9.719,22";
//  ptr += "s2.257,1.01,2.257,2.257V39.73C13.934,40.597,15.302,42.554,15.302,44.833z'fill=#F29C21 /></g></svg>";
//  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Temperature</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (int)temp;
  ptr += "<span class='superscript'>&deg;C</span></div>";
  ptr += "</div>";

  ptr += "<div class='data hum'>";
//  ptr += "<div class='side-by-side icon'>";
//  ptr += "<svg enable-background='new 0 0 19.438 54.003'height=54.003px id=Layer_1 version=1.1 viewBox='0 0 19.438 54.003'width=19.438px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M11.976,8.82v-2h4.084V6.063C16.06,2.715,13.345,0,9.996,0H9.313C5.965,0,3.252,2.715,3.252,6.063v30.982";
//  ptr += "C1.261,38.825,0,41.403,0,44.286c0,5.367,4.351,9.718,9.719,9.718c5.368,0,9.719-4.351,9.719-9.718";
//  ptr += "c0-2.943-1.312-5.574-3.378-7.355V18.436h-3.914v-2h3.914v-2.808h-4.084v-2h4.084V8.82H11.976z M15.302,44.833";
//  ptr += "c0,3.083-2.5,5.583-5.583,5.583s-5.583-2.5-5.583-5.583c0-2.279,1.368-4.236,3.326-5.104V24.257C7.462,23.01,8.472,22,9.719,22";
//  ptr += "s2.257,1.01,2.257,2.257V39.73C13.934,40.597,15.302,42.554,15.302,44.833z'fill=#F29C21 /></g></svg>";
//  ptr += "<i class='fas fa-tint' style='color:blue;'></i>";
//  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Humidity</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (int)humid;
  ptr += "<span>%</span></div>";
  ptr += "</div>";

  //    ptr +="<div class='data Body Temperature'>";
  //    ptr +="<div class='side-by-side icon'>";
  //    ptr +="<svg enable-background='new 0 0 19.438 54.003'height=54.003px id=Layer_1 version=1.1 viewBox='0 0 19.438 54.003'width=19.438px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M11.976,8.82v-2h4.084V6.063C16.06,2.715,13.345,0,9.996,0H9.313C5.965,0,3.252,2.715,3.252,6.063v30.982";
  //    ptr +="C1.261,38.825,0,41.403,0,44.286c0,5.367,4.351,9.718,9.719,9.718c5.368,0,9.719-4.351,9.719-9.718";
  //    ptr +="c0-2.943-1.312-5.574-3.378-7.355V18.436h-3.914v-2h3.914v-2.808h-4.084v-2h4.084V8.82H11.976z M15.302,44.833";
  //    ptr +="c0,3.083-2.5,5.583-5.583,5.583s-5.583-2.5-5.583-5.583c0-2.279,1.368-4.236,3.326-5.104V24.257C7.462,23.01,8.472,22,9.719,22";
  //    ptr +="s2.257,1.01,2.257,2.257V39.73C13.934,40.597,15.302,42.554,15.302,44.833z'fill=#F29C21 /></g></svg>";
  //    ptr +="</div>";
  //    ptr +="<div class='side-by-side text'>Body Temperature</div>";
  //    ptr +="<div class='side-by-side reading'>";
  //    ptr +=(int)bodytemperature;
  //    ptr +="<span class='superscript'>&deg;C</span></div>";
  //    ptr +="</div>";

  ptr += "</div>";
  ptr += "</body>";
  ptr += "</html>";
  return ptr;
}
