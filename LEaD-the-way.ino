#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <ArduinoOTA.h>

#include <FastLED.h>

#define SSID_WIFI "Coldspot"
#define PW_WIFI "Hotstoppassword"

#define FB_APIKEY "AIzaSyAHtWE38JhTMDdpVXOleN8jjWL9o6VFR0Q"
#define FB_DATABASEURL "https://lead-the-way-dac45-default-rtdb.europe-west1.firebasedatabase.app/"


#define NUM_LEDS 94
#define RAINBOW_LENGHT 0.2  //sinus length factor for the whole stripe, 0-1

//switch case
#define RAINBOW 1
#define CONSTCOLOUR 2
#define RAINDROP 3

#define RAIN_LATENCY 883
#define RAIN_SPEED 883

//pins
#define LED_PIN 7
#define OD_PIN 6


TaskHandle_t LEDTask;
TaskHandle_t WiFiTask;

//Global Variables
CRGB leds[NUM_LEDS];
float timeCounter;
uint8_t deviceMode = 1;
uint8_t brightnessInCode = 30;
uint8_t red = 51;
uint8_t green = 19;
uint8_t blue = 1;

uint8_t counter = 0;

//Prototypes
int easeOutCirc(int x);
uint8_t getRandom(uint8_t maximum);
uint8_t getFrequencyCount(uint8_t freq);

/**
 * generates circular curve. 0-1000 is rising, 1000-2000 is falling
 * Output given with maximum set RGB Value
 */
int easeOutCirc(int x) {
  return sqrt(1 - pow(((float)x) / 1000 - 1, 2)) * brightnessInCode;
}
/**
 * Gives a random number between 1 and the given number (not more than 999)
 */
uint8_t getRandom(uint8_t maximum) {
  long m = millis();
  long ult = m % 10;
  long pen = m % 1000;
  uint8_t res = (ult * pen) % maximum;
  return res > 0 ? res : getRandom(maximum);
}

uint8_t getFrequencyCount(uint8_t freq) {
  return millis() / freq;
}

void setup() {
  Serial.begin(115200);

  xTaskCreatePinnedToCore(LEDProcess, "LED Task", 10000, NULL, 1, &LEDTask, 1);
  xTaskCreatePinnedToCore(WiFiProcess, "WiFi Task", 20000, NULL, 0, &WiFiTask, 0);
}

void LEDProcess(void* pvParameters) {
  Serial.print("LEDProcess running on core ");
  Serial.println(xPortGetCoreID());
  pinMode(OD_PIN, OUTPUT);

  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);

  while (true) {
    uint8_t LEDAdress = 0;
    switch (deviceMode) {
      case RAINBOW:
        timeCounter = fmod(((double)millis()) / 1000, (2 * PI));
        for (LEDAdress = 0; LEDAdress < NUM_LEDS; LEDAdress++) {  //for every LED on the stripe
          leds[LEDAdress] = CRGB(
            sin(timeCounter + 0 + LEDAdress * RAINBOW_LENGHT) * brightnessInCode / 2 + brightnessInCode / 2,   //RED
            sin(timeCounter + 2 + LEDAdress * RAINBOW_LENGHT) * brightnessInCode / 2 + brightnessInCode / 2,   //GREEN
            sin(timeCounter + 4 + LEDAdress * RAINBOW_LENGHT) * brightnessInCode / 2 + brightnessInCode / 2);  //BLUE
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
        }
        uint8_t randomRain = random(0,179);
        leds[randomRain] = CRGB(50-easeOutCirc(millis()%2000), easeOutCirc(millis()%2000), 0);*/
        break;
      case CONSTCOLOUR:
        for (LEDAdress = 0; LEDAdress < NUM_LEDS; LEDAdress++) {  //for every LED on the stripe
          leds[LEDAdress] = CRGB(red, green, blue);
        }
        break;
      case false:
      default:
        for (LEDAdress = 0; LEDAdress < NUM_LEDS; LEDAdress++) {  //for every LED on the stripe
          leds[LEDAdress] = CRGB(0, 0, 0);
        }
        break;
    }
    //Serial.print("LED print!");
    FastLED.show();
  }
}

void WiFiProcess(void* pvParameters) {
  Serial.print("WiFiProcess running on core ");
  Serial.println(xPortGetCoreID());
  //WiFi Setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID_WIFI, PW_WIFI);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConnected!");

  //Firebase Setup
  FirebaseData fbdo;
  FirebaseAuth auth;
  FirebaseConfig config;
  bool signupOK = false;

  config.api_key = FB_APIKEY;
  config.database_url = FB_DATABASEURL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase begin!");

  //OTA Setup
  ArduinoOTA.setHostname("LEaD-the-way");
  ArduinoOTA.setPassword("esp32");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else  // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
  Serial.println("OTA begin!");

  while (true) {
    //Serial.println("WiFiTask looping!");
    ArduinoOTA.handle();
    if (Firebase.ready() && signupOK) {
      Firebase.RTDB.setInt(&fbdo, "Strip1/brightness", 50);
      Firebase.RTDB.getInt(&fbdo, "Strip1/brightness");
      Serial.print(fbdo.intData());
      if (Firebase.RTDB.getInt(&fbdo, "Strip1/deviceMode")) {
        deviceMode = fbdo.intData();
      }
    }
  }
}

void loop() {}