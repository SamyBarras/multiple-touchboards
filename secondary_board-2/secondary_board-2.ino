// compiler error handling
#include "Compiler_Errors.h"

// touch includes
#include <MPR121.h>
#include <Wire.h>
#define MPR121_ADDR 0x5C
#define MPR121_INT 4

const int triggerPin = A0;
boolean thisTriggerValue = false;
boolean lastTriggerValue = false;

void setup(){  
  Wire.begin();
  MPR121.begin(MPR121_ADDR);
  MPR121.setInterruptPin(MPR121_INT);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  pinMode(triggerPin, INPUT);
  digitalWrite(triggerPin, LOW); // ensure internal pullup is disabled
 
  for(int i=0; i<12; i++){
    MPR121.setTouchThreshold(i, 12);
    MPR121.setTouchThreshold(i, 6);
  }
}

void loop(){
  processInputs();
  thisTriggerValue = digitalRead(triggerPin);
  if(thisTriggerValue && !lastTriggerValue){ // rising edge triggered
    sendSerialStatus();
  }
  lastTriggerValue = thisTriggerValue;
}

void processInputs() {
  if(MPR121.touchStatusChanged()){    
    MPR121.updateTouchData();
  }
}

void sendSerialStatus(){
  Serial1.begin(9600);
  Serial1.write('T');
    for(int i=0; i<12; i++){
      if(MPR121.getTouchData(i)){
        Serial1.write('1');
      } else {
        Serial1.write('0');
      }
    }
  Serial1.end();
}
