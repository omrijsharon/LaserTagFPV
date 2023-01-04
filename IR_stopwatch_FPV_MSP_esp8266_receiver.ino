#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include "MSP.h"

MSP msp;
unsigned long start_time;
unsigned long total_time;
unsigned long lap_times[3] = {0, 0, 0};
unsigned long best_time;
unsigned long best_lap;
char craft_name[10];
int laps;

// An IR detector/demodulator is connected to GPIO pin 14(D5 on a NodeMCU
// board).
// Note: GPIO 16 won't work on the ESP8266 as it does not have interrupts.
// Note: GPIO 14 won't work on the ESP32-C3 as it causes the board to reboot.
#ifdef ARDUINO_ESP32C3_DEV
const uint16_t kRecvPin = 10;  // 14 on a ESP32-C3 causes a boot loop.
#else  // ARDUINO_ESP32C3_DEV
const uint16_t kRecvPin = 14;
#endif  // ARDUINO_ESP32C3_DEV

IRrecv irrecv(kRecvPin);

decode_results results;

void setup() {
  pinMode(2, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  Serial.begin(115200);
  msp.begin(Serial);
  irrecv.enableIRIn();  // Start the receiver
  Serial.println();
  Serial.print("IRrecvDemo is now running and waiting for IR message on Pin ");
  Serial.println(kRecvPin);
  laps=0;
  total_time = 0;
  sprintf(craft_name, "0-00:00.0");
  msp.command(MSP_SET_NAME, craft_name, sizeof(craft_name), false);
  digitalWrite(2, LOW);  // Turn the LED off by making the voltage HIGH

}

void loop() {
  if (laps>0){
    unsigned long elapsed_time = millis() - start_time;
    int minutes = elapsed_time / 60000;
    int seconds = (elapsed_time / 1000) % 60;
    int milliseconds = elapsed_time % 1000;
    if ((milliseconds%(100)) == 0){
        if (laps==1) {
          sprintf(craft_name, "%01d-%02d:%02d.%01d", laps, minutes, seconds, milliseconds);
          msp.command(MSP_SET_NAME, craft_name, sizeof(craft_name), false);
          // Serial.println(craft_name);  
        }
        else if (laps<=3) {
          sprintf(craft_name, "%01d-%02d:%02d.%01d", laps, minutes, seconds, milliseconds);
          if (seconds > 2) {
            msp.command(MSP_SET_NAME, craft_name, sizeof(craft_name), false);
            // Serial.println(craft_name);  
          }
        }
        else if (laps==4){
          msp.command(MSP_SET_NAME, craft_name, sizeof(craft_name), false);
          // Serial.println(craft_name); 
          delay(3000);
          laps += 1;
        }
        else if (laps==5){
          minutes = total_time / 60000;
          seconds = (total_time / 1000) % 60;
          milliseconds = total_time % 1000;
          sprintf(craft_name, "  %02d:%02d.%01d", minutes, seconds, milliseconds);
          msp.command(MSP_SET_NAME, craft_name, sizeof(craft_name), false);
          // Serial.println(craft_name); 
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
          sprintf(craft_name, "%01d-%02d:%02d.%01d", best_lap, minutes, seconds, milliseconds);
          msp.command(MSP_SET_NAME, craft_name, sizeof(craft_name), false);
          // Serial.println(craft_name); 
          laps += -1;
          delay(1000);
        }
    }
  }

  if (irrecv.decode(&results)) {
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
    irrecv.resume();  // Receive the next value
  }
}
