#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FastLED.h>

// Create display object, this uses ultra-fast I2C clock: 800kHz
Adafruit_SSD1306 lcd(128, 64, &Wire, -1);

const int micpin = A5;
#define PIN 25 //for ring lights
#define PIN2 A16 //for ring lights
#define QSIZE 13  // filter size
#define LED_PIN A13 // LED are connected to A13
#define N_PIXELS 142 // Number of LED 
#define N 100 // Number of samples 
#define fadeDelay 10 // fade amount
#define noiseLevel 15 // Amount of noise we want to chop off 

#define BUTTON B0
int strip_mode = 1;

#define NUM_LEDS 142      /* The amount of pixels/leds you have */
#define DATA_PIN A13       /* The pin your data line is connected to */
#define LED_TYPE WS2812B /* I assume you have WS2812B leds, if not just change it to whatever you have */
#define BRIGHTNESS 255   /* Control the brightness of your leds */
#define SATURATION 255   /* Control the saturation of your leds */
uint8_t hue = 0;

CRGB leds[NUM_LEDS];
CRGB ring1[16];
CRGB ring2[16];
//
//#define ringshow_noglitch() {delay(1);ring.show();delay(1);ring.show();}
//#define ringshow_noglitch2() {delay(1);ring2.show();delay(1);ring2.show();}
//Adafruit_NeoPixel ring = Adafruit_NeoPixel(16, PIN, NEO_GRB + NEO_KHZ800);
//Adafruit_NeoPixel ring2 = Adafruit_NeoPixel(16, PIN2, NEO_GRB + NEO_KHZ800);


int samples[N]; // storage for a sample
int periodFactor = 0; // For period calculation
int t1 = -1;
int T;
int slope;
byte periodChanged = 0;
int mic; // variable for mic analogRead
const int baseline = 1900;
int amp; //different between mic reading and the amplitude
unsigned long timeout = 0;

int queue[QSIZE]; // a queue to store most recent samples
int queue_index = 0;

void setup() {
  Serial.begin(9600);
  lcd.begin(SSD1306_SWITCHCAPVCC, 0x3C); // init
  lcd.clearDisplay();
  lcd.display();
  delay(2000);
//
//  ring.begin();
//  ring2.begin();
//
//  ring.setBrightness(32);
//  ring2.setBrightness(32);
//
//  ring.clear(); // clear all pixels
//  ring2.clear(); // clear all pixels
  
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.addLeds<WS2812B, A16, GRB>(ring1, NUM_LEDS);
  FastLED.addLeds<WS2812B, 25, GRB>(ring2, NUM_LEDS);
  FastLED.clear();
//
//  ring.setPixelColor(0, 0);
//  ring2.setPixelColor(0, 0);
//  ringshow_noglitch();
//  ringshow_noglitch2();
}

float getAvg() {
  float sum = 0.0;
  for (int i = 0; i < QSIZE; i++) {
    sum += queue[i];
  }
  return ((float)sum / QSIZE);
}

void loop() {
  mic = analogRead(micpin);
  int sensorValue = analogRead(micpin);  // Read the analog input from the sound sensor
  amp = abs(mic - baseline);
  queue[queue_index] = amp;
  queue_index = (queue_index + 1) % QSIZE;
  int avg = getAvg();
  Serial.println(avg);

  //lcd face 
  if (avg > 500) {
    lcd.clearDisplay();
    drawHappyFace(48, 16);
  }
  else {
    lcd.clearDisplay();
    drawSadFace(48, 16);
  }
    
  //button debounce
  if (millis() > (timeout+100)) {
    int button1 = digitalRead(BUTTON);
    Serial.println(button1);
    if (button1 == 0) {
      strip_mode *= -1;
    }
   timeout = millis();
  }

  // Map the filtered sensor value to the number of LEDs
   int mappedValue = map(avg, 40, 1000, 0, N_PIXELS / 2);

  if (strip_mode == 1) {
    for (int i = 0; i < N_PIXELS; i++) {
      if (i < mappedValue && (i < (N_PIXELS / 2))) {
        leds[i] = 0x00FF00;  // Change the color as needed (here: red)
      } else if (i > (N_PIXELS - mappedValue - 1)) {
        leds[i] = 0xFF0000;  // Change the color as needed (here: red)
      }
      else {
        leds[i] = 0xFFFFFF;  // Change the color as needed (here: red)
      }
    }
  } 
  else {
    rainbowCycle(1);
  }
  
  mappedValue = map(avg, 100, 1000, 0, 16);

  for(int i=0;i<(16);i++) {
    if (i < mappedValue) {
        ring1[i] = 0xFFFF00;
      } else {
        ring1[i] = 0xFFFFFF;
      }
  }
  
  for(int i=0;i<(16);i++) {
    if (i > (16-mappedValue-1)) {
      ring2[i] = 0xFF00FF;
    } else {
      ring2[i] = 0xFFFFFF;
    }
  }
  
  FastLED.setBrightness(32);
  FastLED.show();

}



void calculatePeriod(int i) {
  if (t1 == -1) {

    t1 = i;
  }
  else {

    int period = periodFactor * (i - t1);
    periodChanged = T == period ? 0 : 1;
    T = period;
    // Serial.println(T);

    t1 = i;
    periodFactor = 0;
  }
}

void drawHappyFace(int x, int y) {
  lcd.clearDisplay();

  // Draw face
  lcd.drawCircle(x + 16, y + 16, 15, WHITE);

  // Draw eyes
  lcd.fillCircle(x + 10, y + 10, 2, WHITE);
  lcd.fillCircle(x + 22, y + 10, 2, WHITE);

  // Draw mouth
  lcd.drawLine(x + 10, y + 20, x + 22, y + 20, WHITE);

  lcd.display();
}

void drawSadFace(int x, int y) {
  lcd.clearDisplay();

  // Draw face
  lcd.drawCircle(x + 16, y + 16, 15, WHITE);

  // Draw eyes
  lcd.fillCircle(x + 10, y + 10, 2, WHITE);
  lcd.fillCircle(x + 22, y + 10, 2, WHITE);

  // Draw mouth (approximate an arc with lines)
  lcd.drawLine(x + 10, y + 24, x + 12, y + 22, WHITE);
  lcd.drawLine(x + 12, y + 22, x + 14, y + 20, WHITE);
  lcd.drawLine(x + 14, y + 20, x + 16, y + 19, WHITE);
  lcd.drawLine(x + 16, y + 19, x + 18, y + 20, WHITE);
  lcd.drawLine(x + 18, y + 20, x + 20, y + 22, WHITE);
  lcd.drawLine(x + 20, y + 22, x + 22, y + 24, WHITE);

  lcd.display();
}

void rainbowCycle(int wait) {
  uint8_t hue = beatsin8(10, 0x0, 0xFF);
  fill_rainbow(leds, NUM_LEDS, hue, 0);
  FastLED.show();
}
