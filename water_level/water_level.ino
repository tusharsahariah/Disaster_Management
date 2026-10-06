#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define TRIG_PIN 9
#define ECHO_PIN 10

const float SAFE_POINT = 8.5;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


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
  float distance = getAverage();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("DISTANCE:");
  display.print(distance);
  display.print(" cm");

  if(distance<= SAFE_POINT){
    display.println();
    display.println("ALERT ALERT");
  }
  else{
    display.println();
    display.println("NORMAL");
  }
  display.display();

  delay(500);

}
