#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FastLED.h>
#include <ESP32Servo.h>

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

const int sweepDuration = 5; // Adjust the speed of the sweep in milliseconds
unsigned long previousMillis = 0;
bool isForwardSweep = true;

Servo servo;

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
  
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.addLeds<WS2812B, A16, GRB>(ring1, NUM_LEDS);
  FastLED.addLeds<WS2812B, 25, GRB>(ring2, NUM_LEDS);
  FastLED.clear();

  servo.attach(18); 
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
  //Serial.println(avg);

  sweepServo();

  //lcd face 
  
  if (avg > 700){
    lcd.clearDisplay();
    drawHappyFace(48, 16);
  }
  else if (avg > 400) {
    lcd.clearDisplay();
    drawMidFace(48, 16);
  }else {
    lcd.clearDisplay();
    drawSadFace(48, 16);
  }
    
  //button debounce
  if (millis() > (timeout+150)) {
    int button1 = digitalRead(BUTTON);
    Serial.println(button1);
    if (button1 == 0) {
      if (strip_mode == 4){
        strip_mode = 1;
      } else{
        strip_mode += 1;
      }
      
    }
   timeout = millis();
  }

  FastLED.setBrightness(32);

  int mappedValue = map(avg, 100, 1000, 0, 16);
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

  // Map the filtered sensor value to the number of LEDs
  mappedValue = map(avg, 40, 1000, 0, N_PIXELS / 2);

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
    FastLED.show();
  } 
  else if (strip_mode == 2) {
    mappedValue = map(avg, 40, 1000, 0, 255);
    Fire(20, mappedValue, 10);

  } else if (strip_mode == 3){
    rainbowCycle(1);
    FastLED.show();
  } else{
    int delay_time = 0;
    mappedValue = map(avg, 40, 1000, 0, 39);
    if ((40-mappedValue)<0){
      delay_time = 0;
    } else{
      delay_time = 40-mappedValue;
    }
    shootingStarAnimation(255, 255, 255, 100, delay_time, 10000, 1);
  }

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
  lcd.drawLine(x + 10, y + 22, x + 12, y + 24, WHITE);
  lcd.drawLine(x + 12, y + 24, x + 14, y + 26, WHITE);
  lcd.drawLine(x + 14, y + 26, x + 16, y + 27, WHITE);
  lcd.drawLine(x + 16, y + 27, x + 18, y + 26, WHITE);
  lcd.drawLine(x + 18, y + 26, x + 20, y + 24, WHITE);
  lcd.drawLine(x + 20, y + 24, x + 22, y + 22, WHITE);

  lcd.display();
}


void drawMidFace(int x, int y) {
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
  // Draw mouth
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

void sweepServo() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= sweepDuration) {
    previousMillis = currentMillis;

    static int angle = 0;

    if (isForwardSweep) {
      servo.write(angle);  // Set the servo position
      angle++;

      if (angle > 180) {
        angle = 180;
        isForwardSweep = false; // Change direction to backward sweep
      }
    } else {
      servo.write(angle);  // Set the servo position
      angle--;

      if (angle < 0) {
        angle = 0;
        isForwardSweep = true; // Change direction to forward sweep
      }
    }
  }
}


//The following code is from Electriangle on GitHub
unsigned long previousMillis2 = 0;           // Stores last time LEDs were updated
int count = 0;                              // Stores count for incrementing up to the NUM_LEDs

void shootingStarAnimation(int red, int green, int blue, int tail_length, int delay_duration, int interval, int direction){
  unsigned long currentMillis = millis();   // Get the time
  if (currentMillis - previousMillis2 >= interval) {
    previousMillis2 = currentMillis;         // Save the last time the LEDs were updated
    count = 0;                              // Reset the count to 0 after each interval
  }
  if (direction == -1) {        // Reverse direction option for LEDs
    if (count < NUM_LEDS) {
      leds[NUM_LEDS - (count % (NUM_LEDS+1))].setRGB(red, green, blue);    // Set LEDs with the color value
      count++;
    }
  }
  else {
    if (count < NUM_LEDS) {     // Forward direction option for LEDs
      leds[count % (NUM_LEDS+1)].setRGB(red, green, blue);    // Set LEDs with the color value
      count++;
    }
  }
  fadeToBlackBy(leds, NUM_LEDS, tail_length);                 // Fade the tail LEDs to black
  FastLED.show();
  delay(delay_duration);                                      // Delay to set the speed of the animation
}

void Fire(int FlameHeight, int Sparks, int DelayDuration) {
  static byte heat[NUM_LEDS];
  int cooldown;
  
  // Cool down each cell a little
  for(int i = 0; i < NUM_LEDS; i++) {
    cooldown = random(0, ((FlameHeight * 10) / NUM_LEDS) + 2);
   
    if(cooldown > heat[i]) {
      heat[i] = 0;
    }
    else {
      heat[i] = heat[i] - cooldown;
    }
  }
  
  // Heat from each cell drifts up and diffuses slightly
  for(int k = (NUM_LEDS - 1); k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
  
  // Randomly ignite new Sparks near bottom of the flame
  if(random(255) < Sparks) {
    int y = random(7);
    heat[y] = heat[y] + random(160, 255);
  }
  
  // Convert heat to LED colors
  for(int j = 0; j < NUM_LEDS; j++) {
    setPixelHeatColor(j, heat[j]);
  }
  
  FastLED.show();
  delay(DelayDuration);
}

void setPixelHeatColor(int Pixel, byte temperature) {
  // Rescale heat from 0-255 to 0-191
  byte t192 = round((temperature / 255.0) * 191);
  
  // Calculate ramp up from
  byte heatramp = t192 & 0x3F; // 0...63
  heatramp <<= 2; // scale up to 0...252
  
  // Figure out which third of the spectrum we're in:
  if(t192 > 0x80) {                    // hottest
    leds[Pixel].setRGB(255, 255, heatramp);
  }
  else if(t192 > 0x40) {               // middle
    leds[Pixel].setRGB(255, heatramp, 0);
  }
  else {                               // coolest
    leds[Pixel].setRGB(heatramp, 0, 0);
  }
}
