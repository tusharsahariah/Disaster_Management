#include <Wire.h>
#include <MPU6050.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

MPU6050 mpu;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

unsigned long eventStartTime = 0;
unsigned long lastVibrationTime = 0;

const unsigned long minimumDuration = 2000; 
const unsigned long timeout = 300;
bool eventActive = false;
bool eventConfirmed = false;
float threshold = 0.03;
float peakVibration = 0;
float filteredMag;

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

  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1);
  }
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

  delay(1000);
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
  float vibration = fabs(mag-filteredMag);


  unsigned long currentTime= millis();
  if (vibration > threshold){
    if (!eventActive){
      eventActive = true;
      eventStartTime= currentTime;
      peakVibration = vibration;
    }
    lastVibrationTime = currentTime;
  }
  if (vibration>peakVibration){
    peakVibration= vibration;
  }
  if (eventActive &&
      currentTime - lastVibrationTime > timeout) {
    eventActive = false;
  }
  if (eventActive &&
      currentTime - eventStartTime >=  minimumDuration) {
    eventConfirmed = true;
  }
  else {
    eventConfirmed=false;
  }

  
  display.clearDisplay();

  display.setCursor(0, 0);

  // display.println("ACCELERATION");

  // display.print("X: ");
  // display.println(x, 3);

  // display.print("Y: ");
  // display.println(y, 3);

  // display.print("Z: ");
  // display.println(z, 3);

  display.print("Mag: ");
  display.println(mag, 3);
  display.print("BASE: ");
  display.println(base, 3);
  display.print("VIB: ");
  display.println(vibration, 3);
  display.println();
  display.print("PEAK: ");
  display.println(peakVibration, 3);
  if (eventConfirmed) {
    display.println("EVENT DETECTED!");
    delay(1000);
  }
  else if (eventActive) {
    display.println("MONITORING...");
  }
  else {
    display.println("NORMAL");
  }
  display.display();

  delay(10);
}