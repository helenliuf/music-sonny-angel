#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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

#define ringshow_noglitch() {delay(1);ring.show();delay(1);ring.show();}
#define ringshow_noglitch2() {delay(1);ring2.show();delay(1);ring2.show();}
Adafruit_NeoPixel ring = Adafruit_NeoPixel(16, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel ring2 = Adafruit_NeoPixel(16, PIN2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

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

  strip.begin();
  ledsOff();
  strip.show();  // Initialize all pixels to 'off'

  ring.begin();
  ring2.begin();
  
  ring.setBrightness(32);
  ring2.setBrightness(32);
  
  ring.clear(); // clear all pixels
  ring2.clear(); // clear all pixels
  
  ring.setPixelColor(0,0);
  ring2.setPixelColor(0,0);
  ringshow_noglitch();
  ringshow_noglitch2(); 
}

float getAvg(){
  float sum = 0.0; 
  for(int i = 0; i < QSIZE; i++){
    sum += queue[i];
  }
  return ((float)sum / QSIZE);
}

void loop() {
  if(millis() > timeout){
    //displayColor(Wheel(100));

    mic = analogRead(micpin);
    int sensorValue = analogRead(micpin);  // Read the analog input from the sound sensor
    
    //Serial.println(mic);
    amp = abs(mic-baseline);
    //Serial.println(amp);
    //int prevValue = queue[queue_index];

    queue[queue_index] = amp;
    queue_index = (queue_index + 1) % QSIZE;
    int avg = getAvg();
    Serial.println(avg);
   
   if(avg > 500){
    lcd.clearDisplay();
    drawHappyFace(48, 16);
   }
   else{
    lcd.clearDisplay();
    drawSadFace(48, 16);
   }
   
   // Map the filtered sensor value to the number of LEDs
   int mappedValue = map(avg, 40, 1000, 0, N_PIXELS/2);
  
   for (int i = 0; i < N_PIXELS; i++) {
     if (i < mappedValue && (i < (N_PIXELS/2))) {
        strip.setPixelColor(i, strip.Color(0, 255, 0));  // Change the color as needed (here: red)
     } else if (i > (N_PIXELS - mappedValue - 1)){
        strip.setPixelColor(i, strip.Color(255, 0, 0));
     }
       else {
        strip.setPixelColor(i, strip.Color(255, 255, 255));  // Turn off the LEDs beyond the mapped value
     }
  }
  strip.show();  
  ring.clear();
  mappedValue = map(avg, 100, 1000, 0, 16);
  
  for(int i=0;i<(ring.numPixels());i++) {
    if (i < mappedValue) {
        ring.setPixelColor(i, ring.Color(255, 255, 0));
      } else {
        ring.setPixelColor(i, ring.Color(255, 255, 255));  // Turn off the LEDs beyond the mapped value
      }
    //ringshow                          `~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~_noglitch();  
    delay(1);
  }
  ringshow_noglitch();

  ring2.clear();
  for(int i=0;i<(ring.numPixels());i++) {
    if (i > (16-mappedValue-1)) {
        ring2.setPixelColor(i, ring2.Color(255, 0, 255));
      } else {
        ring2.setPixelColor(i, ring2.Color(255, 255, 255));  // Turn off the LEDs beyond the mapped value
      }
    //ringshow                          `~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~_noglitch();  
    delay(1);
  }
  ringshow_noglitch2();
  delay(10);  // Adjust the delay for responsiveness
  }
  
}

void calculatePeriod(int i) {
  if(t1 == -1) {
  
    t1 = i;
  }
  else {
    
    int period = periodFactor*(i - t1);
    periodChanged = T==period ? 0 : 1;
    T = period;
  // Serial.println(T);
   
    t1 = i;
    periodFactor = 0;
  }
}

uint32_t getColor(int period) {
  if(period == -1)
    return Wheel(0);
  else if(period > 400)
    return Wheel(5);
  else
    return Wheel(map(-1*period, -400, -1, 50, 255));
}

void fadeOut()
{
  strip.setBrightness(32);
  strip.show();
  /*for(int i=0; i<5; i++) {
    //strip.setBrightness(110 - i*20);
    strip.show(); // Update strip
    //delay(fadeDelay);
    periodFactor +=fadeDelay;
  }*/
}

void fadeIn() {
  strip.setBrightness(32);
  strip.show();
  /*for(int i=0; i<5; i++) {
    //strip.setBrightness(20*i + 30);
    //strip.show();
    //delay(fadeDelay);
    periodFactor+=fadeDelay;
  }*/
}

void ledsOff() {
  fadeOut();
  for(int i=0; i<N_PIXELS; i++) {
    strip.setPixelColor(i, 0, 0, 0);
  }
}

void displayColor(uint32_t color) {
  for(int i=0; i<N_PIXELS; i++) {
    strip.setPixelColor(i, color);
  }
  fadeIn();
}

uint32_t Wheel(byte WheelPos) {
  // Serial.println(WheelPos);
  if(WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } 
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } 
  else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
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
