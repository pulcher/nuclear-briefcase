/*********************************************************************
 This is an example for our nRF51822 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

#include <string.h>
#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include <Adafruit_NeoPixel.h>

#include "BluefruitConfig.h"
#include "blu-clear-briefcase.h"

#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif

/*=========================================================================
    APPLICATION SETTINGS

    FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
   
                              Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                              running this at least once is a good idea.
   
                              When deploying your project, however, you will
                              want to disable factory reset by setting this
                              value to 0.  If you are making changes to your
                              Bluefruit LE device via AT commands, and those
                              changes aren't persisting across resets, this
                              is the reason why.  Factory reset will erase
                              the non-volatile memory where config data is
                              stored, setting it back to factory default
                              values.
       
                              Some sketches that require you to bond to a
                              central device (HID mouse, keyboard, etc.)
                              won't work at all with this feature enabled
                              since the factory reset will clear all of the
                              bonding data stored on the chip, meaning the
                              central device won't be able to reconnect.
    MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
    MODE_LED_BEHAVIOUR        LED activity, valid options are
                              "DISABLE" or "MODE" or "BLEUART" or
                              "HWUART"  or "SPI"  or "MANUAL"
    -----------------------------------------------------------------------*/
    #define FACTORYRESET_ENABLE         1
    #define MINIMUM_FIRMWARE_VERSION    "0.6.6"
    #define MODE_LED_BEHAVIOUR          "MODE"
/*=========================================================================*/


Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
Adafruit_NeoPixel panel(BCB_NUM_LEDS, BCB_NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

// function prototypes over in packetparser.cpp
uint8_t readPacket(Adafruit_BLE *ble, uint16_t timeout);
float parsefloat(uint8_t *buffer);
void printHex(const uint8_t * data, const uint32_t numBytes);
void handleBluToothPacket(Adafruit_BLE *ble, uint8_t len);
void runAnimation();

// the packet buffer
extern uint8_t packetbuffer[];

// animation trackers
uint8_t  currentAnimation;
uint8_t  currentBrightness = 80;
uint32_t currentColor;
uint32_t color1 = BCB_DEFAULT_COLOR_1;
uint32_t color2 = BCB_DEFAULT_COLOR_2;
int      currentColorIndex = 0;
uint8_t  animationMode;

int lastPixel = 0;

float smoothness_pts = 100;

int pixelQueue = 0;

void setup(void)
{
  while (!Serial); 
  delay(500);

  Serial.begin(115200);
  Serial.println(F("Blu-clear briefcase app"));
  Serial.println(F("-----------------------------------------"));

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  // ble.name("Blu-clear Briefcase");
  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in Controller mode"));
  Serial.println(F("Then activate/use the sensors, color picker, game controller, etc!"));
  Serial.println();

  ble.verbose(false);  // debug info is a little annoying after this point!

  Serial.println(F("******************************"));

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }

  // Set Bluefruit to DATA mode
  Serial.println( F("Switching to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);

  Serial.println(F("******************************"));

  currentAnimation = BCB_ANIMATION_BREATHING;
  //animationMode = BCB_SINGLE_ANIMATION;

  panel.begin();            // INITIALIZE NeoPixel strip object (REQUIRED)
  panel.setBrightness(80);  // figure out what the inital setting should be.
  panel.show();             // Turn OFF all pixels ASAP

  pinMode(BCB_NEOPIXEL_PIN, OUTPUT);
  digitalWrite(BCB_NEOPIXEL_PIN, LOW);
}

void loop(void)
{
  /* Wait for new data to arrive */
  uint8_t len = readPacket(&ble, BCB_READPACKET_TIMEOUT);

  handleBluToothPacket(&ble, len);

  runAnimation();

}

void runAnimation()
{
  // Serial.print("currentAnimation: ");
  // Serial.print(currentAnimation);
  // Serial.print(" currentBrightness: ");
  // Serial.println(currentBrightness);

  panel.setBrightness(currentBrightness);

  switch (currentAnimation) {
    case BCB_ANIMATION_BREATHING:
      gaussianWaveBreathing();
      return;
    case BCB_ANIMATION_COLOR_WIPE:
      colorWipe();
      return;
    case BCB_ANIMATION_PANEL_WIPE:
      panelWipe();
      return;
    case BCB_ANIMATION_THEATER:
      theaterChase();
      return;
    default:
      return;
  }
}


uint32_t getNextColor(int pixelNumber) 
{
  if (pixelNumber < 256)
    return color1;
  if (pixelNumber > 255 && pixelNumber < 512)
    return color2;
  
  return color1;  
}

uint32_t getNextColorInv(int pixelNumber) 
{
  if (pixelNumber < 256)
    return color2;
  if (pixelNumber > 255 && pixelNumber < 512)
    return color1;
  
  return color2;  
}

/*****************************************************************
* Animations
******************************************************************/

void colorWipe()
{
  for (int color = 0; color < 2; color++)
  {
    uint32_t currentColor = color % 2 ? color1 : color2;
    for (int i = 0; i < panel.numPixels(); i++)
    {                                       // For each pixel in strip...
      panel.setPixelColor(i, currentColor); //  Set pixel's color (in RAM)
      //delay(50);                   //  Pause for a moment
    }

    panel.show();                         //  Update strip to match
    delay(200);
  }
}

void gaussianWaveBreathing()
{
  float gamma = 0.20; // affects the width of peak (more or less darkness)
  float beta = 0.5;   // shifts the gaussian to be symmetric

  for (int color = 0; color < 2; color++)
  {
    uint32_t currentColor = color % 2 ? color1 : color2;

    for (int ii = 0; ii < smoothness_pts; ii++)
    {
      float pwm_val = BCB_MAX_BRIGHT * (exp(-(pow(((ii / smoothness_pts) - beta) / gamma, 2.0)) / 2.0));
      if (pwm_val < BCB_MIN_BRIGHT)
      {
        pwm_val = BCB_MIN_BRIGHT;
      }

      // Serial.println(int(pwm_val));
      panel.setBrightness(pwm_val);

      for (int i = 0; i < BCB_NUM_LEDS; i++)
      {
        panel.setPixelColor(i, currentColor);
      }

      panel.show();
    }
  }
}

void theaterChase() {
  for (int color = 0; color < 2; color++) {
    uint32_t currentColor = color % 2 ? color1 : color2;
    for (int a = 0; a < 10; a++) {   // Repeat 10 times...
      for (int b = 0; b < 3; b++) {  //  'b' counts from 0 to 2...
        panel.clear();               //   Set all pixels in RAM to 0 (off)
        // 'c' counts up from 'b' to end of strip in steps of 3...
        for (int c = b; c < panel.numPixels(); c += 3) {
          panel.setPixelColor(c, currentColor);  // Set pixel 'c' to value 'color'
        }
        panel.show();  // Update strip with new contents
        delay(5);      // Pause for a moment
      }
    }
  }
}

void panelWipe()
{
  for (int color = 0; color < 2; color++)
  {
    uint32_t currentColor = color % 2 ? color1 : color2;
    
    for (int i = 0; i < panel.numPixels(); i++)
    {                        
      uint32_t panelColor = color % 2 ? getNextColor(i) : getNextColorInv(i);
      panel.setPixelColor(i, panelColor); //  Set pixel's color (in RAM)
    }

  // i % 256  => 0   % 2

    panel.show();                         //  Update strip to match
    delay(100);
  }
}

void checkBrightness()
{
  currentBrightness = constrain(currentBrightness, BCB_MIN_BRIGHT, BCB_MAX_BRIGHT);
}

void brighter()
{
  currentBrightness += 10;
  checkBrightness();
}

void dimmer()
{
  currentBrightness -= 10;
  checkBrightness();
}

void changeCurrentAnimation(int buttonNumber)
{
  switch(buttonNumber) 
  {
    case 1:
      currentAnimation = BCB_ANIMATION_BREATHING;
      return;
    case 2:
      currentAnimation = BCB_ANIMATION_COLOR_WIPE;
      return;
    case 3:
      currentAnimation = BCB_ANIMATION_PANEL_WIPE;
      return;
    case 4:
      currentAnimation = BCB_ANIMATION_THEATER;
      return;
    case 5:
      // up arrow
      brighter();
      return;
    case 6:
      // down arrow
      dimmer();
      return;
    case 7:
      // left arrow
      currentAnimation = BCB_ANIMATION_STOP;
      return;
    case 8:
      // right arrow
      return;
    default:
      currentAnimation = -1;
  }
}

void handleBluToothPacket(Adafruit_BLE *ble, uint8_t len)
{
  if (len == 0) return;
  /* Got a packet! */
  printHex(packetbuffer, len);

  // Color
  if (packetbuffer[1] == 'C') {
    uint8_t red = packetbuffer[2];
    uint8_t green = packetbuffer[3];
    uint8_t blue = packetbuffer[4];
    Serial.print ("RGB #");
    if (red < 0x10) Serial.print("0");
    Serial.print(red, HEX);
    if (green < 0x10) Serial.print("0");
    Serial.print(green, HEX);
    if (blue < 0x10) Serial.print("0");
    Serial.println(blue, HEX);

    color2 = color1;

    color1 = (red << 16) + (green << 8) + blue;

    Serial.print("color1: ");
    Serial.print(color1, HEX);
    Serial.print(" color2: ");
    Serial.print(color2, HEX);
  }

  // Buttons
  if (packetbuffer[1] == 'B') {
    int buttnum = packetbuffer[2] - '0';
    boolean pressed = packetbuffer[3] - '0';
    
    Serial.print ("Button "); 
    Serial.print(buttnum);
    if (pressed) {
      Serial.println(" pressed");
    } else {
      Serial.println(" released");
      changeCurrentAnimation(buttnum);
    }
  }
}
