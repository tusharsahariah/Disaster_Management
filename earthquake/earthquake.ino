#include <Wire.h>
#include <MPU6050.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "secret.h"

//Oled
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//MPU
MPU6050 mpu;

//Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;


unsigned long eventStartTime = 0;
unsigned long lastVibrationTime = 0;
unsigned long previousCrossingTime = 0;
unsigned long eventDuration = 0;
unsigned long impactStartTime=0;
const unsigned long impactDuration=1000;

const unsigned long minimumDuration = 2000; 
const int minimumCrossings =10;
const unsigned long timeout = 300;
unsigned long earthquakeDisplayStartTime = 0;
const unsigned long earthquakeDisplayDuration = 3000;

bool earthquakeDisplayActive = false;

bool eventActive = false;
bool eventConfirmed = false;
bool impactTriggered = false;

float peakVibration = 0;
float filteredMag;
float dynamicSignal;
float previousSignal = 0;
int zeroCrossings = 0;
float estimatedFrequency =0;
bool earthquakeDetected = false;
bool eventLogged = false;

unsigned long eventID = 0;

float threshold = 0.05;
float requiredPeak = 0.15;
float minFrequency = 0.5;
float maxFrequency = 10.0;
float crossingThreshold = 0.01;

int eventType=0;

float base;
float baseline(){
  int N=100;
  float sum = 0;
  for (int i=0;i<N;i++){
    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);
    float x = ax / 16384.0;
    float y = ay / 16384.0;
    float z = az / 16384.0;
    float mag = sqrt(x*x + y*y + z*z);
    sum+=mag;
    delay(10);
    }
  float baseline= sum/N;
  return baseline;//average of 100 samples
}


void setup() {

  Wire.begin(21,22);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);


  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1);
  }

  while (WiFi.status()!= WL_CONNECTED){
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println("SCANNIGN WIFI....");
    display.display();
    delay(300);
  }

  display.clearDisplay();
  display.setCursor(0,0);
  display.println("WIFI CONNECTED");
  display.println(WiFi.localIP());
  display.display();
  delay(1000);

  //FIREBASE
  config.api_key = FIREBASE_API_KEY;
  config.database_url = FIREBASE_DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase authentication successful");}
    else {
    Serial.print("Firebase authentication failed: ");
    Serial.println(config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase initialized");

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("FIREBASE READY");
  display.display();
  delay(1000);

  //mpu
  mpu.initialize();
  if (!mpu.testConnection()) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("MPU6050");
    display.println("NOT FOUND!");
    display.display();
    while (1);
  }


  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("EARTHQUAKE DETECTOR");
  display.println("VERSION 1.0");
  display.display();
  delay(2000);


  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("CALIBRATING...");
  delay(1000);
  base = baseline();
  filteredMag = base;


  // Display baseline
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("BASELINE:");
  display.print(base, 4);
  display.println(" g");
  display.display();
  delay(2000);

}

void loop() {
  

  int16_t ax, ay, az;

  mpu.getAcceleration(&ax, &ay, &az);
  float x = ax / 16384.0;
  float y = ay / 16384.0;
  float z = az / 16384.0;
  float mag = sqrt(x*x + y*y + z*z);
  filteredMag = 0.95 * filteredMag + 0.05 * mag;
  dynamicSignal =mag-filteredMag;
  float vibration = fabs(dynamicSignal);
  unsigned long currentTime = millis();


  if (vibration > threshold){
    if (!eventActive){
      eventActive = true;
      eventStartTime= currentTime;
      peakVibration = vibration;
      zeroCrossings=0;
      previousCrossingTime = 0;
      estimatedFrequency = 0;
      previousSignal=0;
      eventDuration = 0;
      eventType = 0;
    }
    lastVibrationTime = currentTime;
  }
  if (eventActive){
    eventDuration = currentTime - eventStartTime;
    if ((dynamicSignal > crossingThreshold &&
     previousSignal < -crossingThreshold) ||
    (dynamicSignal < -crossingThreshold &&
     previousSignal > crossingThreshold)){

    zeroCrossings ++;
    unsigned long crossingTime= currentTime;
    if (previousCrossingTime !=0){
      unsigned long halfPeriod = crossingTime - previousCrossingTime;
      estimatedFrequency = 500.0/halfPeriod;
    }
    previousCrossingTime = crossingTime;
  }

  }
  previousSignal= dynamicSignal;
  if (vibration>peakVibration){
    peakVibration= vibration;
  }
  if (vibration >=requiredPeak && !impactTriggered){
    eventType=1;
    impactStartTime=currentTime;
    impactTriggered = true;
  }
  if (eventType == 1 &&
    currentTime - impactStartTime >= impactDuration ) {
    eventType = 0;             
  } 
  if (vibration < threshold) {
    impactTriggered = false;
  }
  if (eventActive &&
      currentTime - lastVibrationTime > timeout) {
      eventActive = false;
      earthquakeDetected = false;
      eventLogged = false;

  }
  if (eventActive &&
    eventDuration >= minimumDuration &&
    peakVibration >= requiredPeak &&
    zeroCrossings >= minimumCrossings &&
    estimatedFrequency >= minFrequency &&
    estimatedFrequency <= maxFrequency) {

    earthquakeDetected = true;
    if (!earthquakeDisplayActive) {
        earthquakeDisplayActive = true;
        earthquakeDisplayStartTime = currentTime;
    }
    }
    else {
        earthquakeDetected = false;
        earthquakeDisplayActive = false;
    }
  if (earthquakeDetected && !eventLogged){
    eventID++;
    eventLogged =true;
    String eventPath = "/events/event_" + String(eventID);
    FirebaseJson eventData;
    eventData.set("peak", peakVibration);
    eventData.set("duration", eventDuration);
    eventData.set("frequency", estimatedFrequency);
    eventData.set("crossings", zeroCrossings);
    eventData.set("detected", true);
    if (Firebase.RTDB.setJSON(&fbdo, eventPath.c_str(), &eventData)) {

          Serial.println("Earthquake event uploaded successfully");

      }
      else {

          Serial.print("Event upload failed: ");
          Serial.println(fbdo.errorReason());

      }
    FirebaseJson currentData;

    currentData.set("detected", true);
    currentData.set("peak", peakVibration);
    currentData.set("duration", eventDuration);
    currentData.set("frequency", estimatedFrequency);
    currentData.set("crossings", zeroCrossings);
    currentData.set("eventID", eventID);

    if (Firebase.RTDB.setJSON(&fbdo, "/current", &currentData)) {

        Serial.println("Current data updated successfully");

    }
    else {

        Serial.print("Current data update failed: ");
        Serial.println(fbdo.errorReason());

    }

}
  // if (eventActive &&
  //     eventDuration >=  minimumDuration) {
  //   eventConfirmed = true;
  // }
  // else {
  //   eventConfirmed=false;
  // }

  
  display.clearDisplay();
  display.setTextSize(1);

  // display.println("ACCELERATION");

  // display.print("X: ");
  // display.println(x, 3);

  // display.print("Y: ");
  // display.println(y, 3);

  // display.print("Z: ");
  // display.println(z, 3);

  display.setCursor(0, 0);
  display.print("Mag:");
  display.println(mag, 3);

  // display.setCursor(64, 0);
  // display.print("BASE:");
  // display.println(base, 3);
  display.setCursor(0, 10);
  display.print("VIB:");
  display.println(vibration, 3);
  display.setCursor(64, 10);
  display.print("PEAK:");
  display.println(peakVibration, 3);

  display.setCursor(0, 20);
  display.print("CROSS:");
  display.println(zeroCrossings);
  display.setCursor(64, 20);
  display.print("FREQ:");
  display.println(estimatedFrequency, 2);

  display.setCursor(0, 32);
  display.print("TYPE:");
  if (eventType == 0) {
    display.print("NORMAL");
    }
  else if(eventType==1){
    display.print("IMPACT");
  }
  display.setCursor(0, 44);
  display.print("EQ:");
  if (earthquakeDisplayActive &&
    currentTime - earthquakeDisplayStartTime < earthquakeDisplayDuration) {
      display.println("DETECTED");
      display.display()
  }
  else {
      display.println("MONITORING.....");
  }
  display.display();

  delay(10);
}