#include "MSP.h"

MSP msp;
uint16_t msp_rc_command[] = {1000, 1000, 1000, 1000, 2000, 1000, 1000, 1000};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  msp.begin(Serial);
  // for (int i=0; i<8; i++){
  //   msp_rc_command[i] = random(1000, 2000);
  // }  
}

void loop() {
  // put your main code here, to run repeatedly:


  msp.command(MSP_SET_RAW_RC, msp_rc_command, sizeof(msp_rc_command), false);
  delay(1);
}
