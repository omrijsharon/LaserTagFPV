#include <Arduino.h>
#include "MSP.h"

MSP msp;
unsigned long start_time;
unsigned long total_time;
unsigned long lap_times[3] = {0, 0, 0};
unsigned long best_time;
unsigned long best_lap;
char craft_name[10];
int laps;


/*
 * Set sensible receive pin for different CPU's
 */
#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny87__) || defined(__AVR_ATtiny167__)
#include "ATtinySerialOut.hpp" // Available as Arduino library "ATtinySerialOut"
#  if defined(ARDUINO_AVR_DIGISPARKPRO)
#define IR_INPUT_PIN    9 // PA3 - on Digispark board labeled as pin 9
#  else
#define IR_INPUT_PIN    0 // PCINT0
#  endif
#elif defined(__AVR_ATtiny1616__)  || defined(__AVR_ATtiny3216__) || defined(__AVR_ATtiny3217__)
#define IR_INPUT_PIN    10
#elif (defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__))
#define IR_INPUT_PIN    21 // INT0
#elif defined(ESP8266)
#define IR_INPUT_PIN    14 // D5
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
#define IR_INPUT_PIN    8
#elif defined(ESP32)
#define IR_INPUT_PIN    15
#elif defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARCH_MBED_NANO)
#define IR_INPUT_PIN    3   // GPIO15 Use pin 3 since pin 2|GPIO25 is connected to LED on Pi pico
#elif defined(ARDUINO_ARCH_RP2040) // Pi Pico with arduino-pico core https://github.com/earlephilhower/arduino-pico
#define IR_INPUT_PIN    15  // to be compatible with the Arduino Nano RP2040 Connect (pin3)
#else
#define IR_INPUT_PIN    3   // INT0
//#define NO_LED_FEEDBACK_CODE   // Activate this if you want to suppress LED feedback or if you do not have a LED. This saves 14 bytes code and 2 clock cycles per interrupt.
#endif

//#define DEBUG // to see if attachInterrupt is used
//#define TRACE // to see the state of the ISR state machine

/*
 * Second: include the code and compile it.
 */
//#define USE_FAST_8_BIT_AND_PARITY_TIMING // Use short protocol. No address and 16 bit data, interpreted as 8 bit command and 8 bit inverted command
#include "TinyIRReceiver.hpp"

/*
 * Helper macro for getting a macro definition as string
 */
#if !defined(STR_HELPER)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#endif

volatile struct TinyIRReceiverCallbackDataStruct sCallbackData;

void setup() {
    Serial.begin(115200);
    msp.begin(Serial);
#if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/|| defined(SERIALUSB_PID) || defined(ARDUINO_attiny3217)
    delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#endif
    // Just to know which program is running on my Arduino
#if defined(ESP8266) || defined(ESP32)
    Serial.println();
#endif
    Serial.println(F("START " __FILE__ " from " __DATE__));
    if (!initPCIInterruptForTinyReceiver()) {
        Serial.println(F("No interrupt available for pin " STR(IR_INPUT_PIN))); // optimized out by the compiler, if not required :-)
    }
#if defined(USE_FAST_8_BIT_AND_PARITY_TIMING)
    Serial.println(F("Ready to receive Fast IR signals at pin " STR(IR_INPUT_PIN)));
#else
    Serial.println(F("Ready to receive NEC IR signals at pin " STR(IR_INPUT_PIN)));
#endif
  laps=0;
  total_time = 0;
  msp.command(MSP_SET_NAME, craft_name, 10, false);
  sprintf(craft_name, "0-00:00.000");
  // msp.command(MSP_SET_NAME, craft_name, 10, false);
  Serial.println(craft_name);  
}

void loop() {
  if (laps>0){
    unsigned long elapsed_time = millis() - start_time;
    int minutes = elapsed_time / 60000;
    int seconds = (elapsed_time / 1000) % 60;
    int milliseconds = elapsed_time % 1000;
    if (milliseconds%(100+random(25)) == 0){
        if (laps==1) {
          sprintf(craft_name, "%01d-%02d:%02d.%02d", laps, minutes, seconds, milliseconds);
        // msp.command(MSP_SET_NAME, craft_name, 10, false);
          Serial.println(craft_name);  
        }
        else if (laps<=3) {
          sprintf(craft_name, "%01d-%02d:%02d.%02d", laps, minutes, seconds, milliseconds);
          if (seconds > 2) {
            // msp.command(MSP_SET_NAME, craft_name, 10, false);
            Serial.println(craft_name);  
          }
        }
        else if (laps==4){
          // msp.command(MSP_SET_NAME, craft_name, 10, false);
          Serial.println(craft_name); 
          delay(3000);
          laps += 1;
        }
        else if (laps==5){
          minutes = total_time / 60000;
          seconds = (total_time / 1000) % 60;
          milliseconds = total_time % 1000;
          sprintf(craft_name, "%02d:%02d.%02d", minutes, seconds, milliseconds);
          // msp.command(MSP_SET_NAME, craft_name, 10, false);
          Serial.println(craft_name); 
          laps += 1;
          delay(1000);
        }
        else if (laps==6){
          best_time = lap_times[0];
          best_lap = 1;
          for (int i=1; i<3; i++){
            if (lap_times[i] < best_time) {
              best_time = lap_times[i];
              best_lap = i+1;
            }
          }
          minutes = best_time / 60000;
          seconds = (best_time / 1000) % 60;
          milliseconds = best_time % 1000;
          sprintf(craft_name, "%01d-%02d:%02d.%02d", best_lap, minutes, seconds, milliseconds);
          // msp.command(MSP_SET_NAME, craft_name, 10, false);
          Serial.println(craft_name); 
          laps += -1;
          delay(1000);
        }
    }
  }
  
    if (sCallbackData.justWritten) {
        sCallbackData.justWritten = false;
#if defined(USE_FAST_8_BIT_AND_PARITY_TIMING)
        Serial.print(F("Command=0x"));
#else
        Serial.print(F("Address=0x"));
        Serial.print(sCallbackData.Address, HEX);
        Serial.print(F(" Command=0x"));
#endif
        Serial.print(sCallbackData.Command, HEX);
        if (sCallbackData.Flags == IRDATA_FLAGS_IS_REPEAT) {
            Serial.print(F(" Repeat"));
        }
        if (sCallbackData.Flags == IRDATA_FLAGS_PARITY_FAILED) {
            Serial.print(F(" Parity failed"));
        }
        Serial.println();
    }
    /*
     * Put your code here
     */
}

/*
 * This is the function is called if a complete command was received
 * It runs in an ISR context with interrupts enabled, so functions like delay() etc. are working here
 */
#if defined(ESP8266) || defined(ESP32)
IRAM_ATTR
#endif

#if defined(USE_FAST_8_BIT_AND_PARITY_TIMING)
void handleReceivedTinyIRData(uint8_t aCommand, uint8_t aFlags)
#else
void handleReceivedTinyIRData(uint8_t aAddress, uint8_t aCommand, uint8_t aFlags)
#endif
        {
#if defined(ARDUINO_ARCH_MBED) || defined(ESP32)
    // Copy data for main loop, this is the recommended way for handling a callback :-)
#  if !defined(USE_FAST_8_BIT_AND_PARITY_TIMING)
    sCallbackData.Address = aAddress;
#  endif
    sCallbackData.Command = aCommand;
    sCallbackData.Flags = aFlags;
    sCallbackData.justWritten = true;
#else
    /*
     * Printing is not allowed in ISR context for any kind of RTOS
     * For Mbed we get a kernel panic and "Error Message: Semaphore: 0x0, Not allowed in ISR context" for Serial.print()
     * for ESP32 we get a "Guru Meditation Error: Core  1 panic'ed" (we also have an RTOS running!)
     */
    // Print only very short output, since we are in an interrupt context and do not want to miss the next interrupts of the repeats coming soon
#  if defined(USE_FAST_8_BIT_AND_PARITY_TIMING)
    printTinyReceiverResultMinimal(aCommand, aFlags, &Serial);
#  else
    if (laps==0){
      start_time = millis();
      laps = 1;
    }
    else if (laps<=3) {
      unsigned long elapsed_time = millis() - start_time;
      if (elapsed_time > 5000) {
        lap_times[laps-1] = elapsed_time;
        start_time = millis();
        laps +=1;
        total_time += elapsed_time;
      }
    }
    // printTinyReceiverResultMinimal(aAddress, aCommand, aFlags, &Serial);
#  endif
#endif
}
