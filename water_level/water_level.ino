#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define TRIG_PIN 9
#define ECHO_PIN 10

// const unsigned long interval=500;
// unsigned long previousMillis=0;

const float EMPTY_DISTANCE = 15.0;
const float FULL_DISTANCE = 3.0;
float distance;



Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


float waterLevelPercent(){
  float levelPercent = ((EMPTY_DISTANCE-distance)/(EMPTY_DISTANCE-FULL_DISTANCE))*100.0;
  levelPercent = constrain(levelPercent, 0, 100);
  return levelPercent;
}

float getDistance(){
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN,HIGH);

  float distance = duration*0.0343/2;
  return distance;
}

float getAverage(){
  float sum = 0;
  int n=10;
  for(int i = 0; i < n;i++){
    sum += getDistance();
    delay(50);
  }
  float average= sum/n;
  return average;
}
void setup() {
  Wire.begin();
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    while(1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("WATER LEVEL");
  display.println("DETECTOR");
  display.display();
  delay(2000);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
  // unsigned long currentMillis =millis();
  // if(currentMillis-previousMillis>=interval){
  //   previousMillis=currentMillis;
  // }
  distance = getAverage();
  float levelPercent = waterLevelPercent();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("DISTANCE:");
  display.print(distance);
  display.print(" cm");
  display.setCursor(0, 30);
  display.print("STATUS:");

  if(levelPercent < 20.0){
    display.println();
    display.println("EMPTY");
  }
  else if(levelPercent <50.0){
    display.println();
    display.println("LOW");
  }
  else if(levelPercent < 75.0){
    display.println();
    display.println("MEDIUM");
  }
  else if(levelPercent <90.0){
    display.println();
    display.println("HIGH");
  }
  else if(levelPercent <95.0){
    display.println();
    display.println("HIGH WATER LEVEL");
  }
  else{
    display.println();
    display.println("OVERFLOW RISK");
  }
  display.display();

  delay(500);

}
