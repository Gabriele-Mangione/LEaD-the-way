#include <FastLED.h>

#define NUM_LEDS      180
#define MAX_RGB_VALUE 25  //1-127, light intensity of the LEDs, higher number draw more current
#define RAINBOW_LENGHT 0.2  //defines how long the periods of the rainbow formula should be (when the rainbow should repeat himself)

//switch case
#define RAINBOW       1
#define CONSTCOLOR    2
#define RAINDROP      3


#define RAIN_LATENCY  883
#define RAIN_SPEED    883

//pins
#define POTPIN        A2
#define LED_PIN       7

CRGB leds[NUM_LEDS];  //creating array for every single led on the stripe, with struct CRGB, which contains one R value, one G value and one B value
/*
 * TODO:
 * struct for rgb
 * if rand = 1 else next
 */
//Global Variables

float timeCounter;
uint8_t LEDAdress = 0;
volatile uint8_t state = RAINBOW;
float potValue;

uint8_t counter=0;

//Prototypes
void changeState();
int easeOutCirc(int x);
uint8_t getRandom(uint8_t maximum);
uint8_t getFrequencyCount(uint8_t freq);

void setup() {
  //settings Timer1 Interrupt
  TCCR1B = B00000101;   //prescaler 1024
  
  
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  Serial.begin(9600);
  pinMode(2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), changeState, RISING);
}

void changeState(){
  if(state < RAINDROP)
    state++;
  else
    state = RAINBOW;
}
/**
 * generates circular curve. 0-1000 is rising, 1000-2000 is falling
 * Output given with maximum set RGB Value
 */
int easeOutCirc(int x){
  return sqrt(1 - pow(((float)x)/1000 - 1, 2)) * MAX_RGB_VALUE*2;
}
/**
 * Gives a random number between 1 and the given number (not more than 999)
 */
uint8_t getRandom(uint8_t maximum){
      long m=millis();
      long ult=m%10;
      long pen=m%1000;
      uint8_t res= (ult*pen)%maximum;
      return res>0? res : getRandom(maximum);
}

uint8_t getFrequencyCount(uint8_t freq){
  return millis()/freq;
}

void loop() {
  
  while(1){
    switch(state){
      case RAINBOW:
        timeCounter = fmod(((double)millis())/1000,(2*PI));
        for(LEDAdress = 0; LEDAdress < NUM_LEDS; LEDAdress++){    //for every LED on the stripe
          leds[LEDAdress] = CRGB(
            sin(timeCounter + 0+LEDAdress*RAINBOW_LENGHT) * MAX_RGB_VALUE + MAX_RGB_VALUE,     //RED
            sin(timeCounter + 2+LEDAdress*RAINBOW_LENGHT) * MAX_RGB_VALUE + MAX_RGB_VALUE,     //GREEN
            sin(timeCounter + 4+LEDAdress*RAINBOW_LENGHT) * MAX_RGB_VALUE + MAX_RGB_VALUE);    //BLUE
        }
        break;
      case RAINDROP:
        /*for(LEDAdress = 0; LEDAdress < NUM_LEDS; LEDAdress++){
          point.green = 50;
          uint8_t nextRound=getFrequencyCount(RAIN_LATENCY);
          point.red=0;
          
          if(nextRound!=counter){
            counter=nextRound;
            //Serial.println(getRandom(180));
            point.red=easeOutCirc(millis()%1000);
            point.green=50-easeOutCirc(millis()%1000);
          }
        }*/
        uint8_t randomRain = random(0,179);
        leds[randomRain] = CRGB(50-easeOutCirc(millis()%2000), easeOutCirc(millis()%2000), 0);
        break;
      default:
        break;
    }
  FastLED.show();
  }
}
