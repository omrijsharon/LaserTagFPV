#include "MSP.h"

MSP msp;
int health = 99;
int ammo = 99;
int t;


void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  Serial.begin(115200);
  msp.begin(Serial);
}

void loop() {
  // put your main code here, to run repeatedly:
  ammo -= 3; 
  health -= 1;
  if (health < 0){
    health = 99;
  }
  if (ammo < 0){
    ammo = 99;
  }
  
  if (health % 2 == 0){
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
  }
  else {
    digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage leveln 
  }
  char *craft_name = (char*)malloc(12);
  // show_ammo_and_health(craft_name, ammo, health);
  char *buffer_health = (char*)malloc(4);
  char *buffer_ammo = (char*)malloc(4);

  itoa(health, buffer_health, 10);

  itoa(ammo, buffer_ammo, 10);

  strcat(craft_name, "H:");
  strcat(craft_name, buffer_health);
  strcat(craft_name, ", A:");
  strcat(craft_name, buffer_ammo);

  // Serial.println(craft_name);
  // msp.command(MSP_SET_NAME, craft_name, sizeof(craft_name), false);
  msp.command(MSP_SET_NAME, craft_name, 12, false);
  free(buffer_health);
  free(buffer_ammo);
  free(craft_name); // deallocate memory once you've done
  delay(500);
}
