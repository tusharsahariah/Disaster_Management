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
unsigned long previousCrossingTime = 0;
unsigned long eventDuration = 0;

const unsigned long minimumDuration = 2000; 
const int minimumCrossings =10;
const unsigned long timeout = 300;

bool eventActive = false;
bool eventConfirmed = false;
float threshold = 0.05;
float peakVibration = 0;
float filteredMag;
float dynamicSignal;
float previousSignal = 0;
int zeroCrossings = 0;
float crossingThreshold = 0.01;
float estimatedFrequency =0;

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

  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);

  display.println("OLED OK!");
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
  if (eventActive &&
      currentTime - lastVibrationTime > timeout) {
    eventActive = false;
    if (eventDuration >= minimumDuration &&
        zeroCrossings >= minimumCrossings) {

        eventType = 2;   // SUSTAINED VIBRATION

    }
    else {

        eventType = 1;   // SHORT IMPACT

    }

  }
  if (eventActive &&
      eventDuration >=  minimumDuration) {
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

  display.print("Mag:");
  display.println(mag, 3);
  display.print("BASE:");
  display.println(base, 3);
  display.print("VIB:");
  display.println(vibration, 3);
  display.setCursor(70,0);
  display.print("PEAK:");
  display.println(peakVibration, 3);
  display.setCursor(70,8);
  display.print("CROSS:");
  display.println(zeroCrossings);
  display.setCursor(70,16);
  display.print("FREQ:");
  display.println(estimatedFrequency, 2);
  display.setCursor(0,48);
  display.setTextSize(1);
  display.print("TYPE:");


if (eventType == 0) {
    display.println("NORMAL");
}
else if (eventType == 1) {
    display.println("IMPACT");
}
else if (eventType == 2) {
    display.println("SUSTAINED");
}
  if (eventConfirmed) {
    display.println("EVENT DETECTED!");
  }
  else if (eventActive) {
    display.println("MONITORING...");
  }
  display.display();

  delay(10);
}