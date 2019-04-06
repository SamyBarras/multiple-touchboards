// compiler error handling
#include "Compiler_Errors.h"

// touch includes
#include <MPR121.h>
#include <Wire.h>
#define MPR121_ADDR 0x5C
#define MPR121_INT 4

#include <Keyboard.h>
#define holdKey false 
const char keyMap[24] = {'A', 'Z', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', 'Q', 'S','D', 'F', 'G', 'H', 'J', 'K', 'L', 'M', 'W', 'X', 'C', 'V'};
// number of boards config
// you can reduce this to improve response time, but the code will work fine with it 
// left at 6 - do not try to increase this beyond 6!
const int numSecondaryBoards = 1;
const int totalNumElectrodes = (numSecondaryBoards+1)*12;

// serial comms definitions
const int serialPacketSize = 13;

// secondary board touch variables
bool thisExternalTouchStatus[numSecondaryBoards][12];
bool lastExternalTouchStatus[numSecondaryBoards][12];

// compound touch variables
bool touchStatusChanged = false;
bool isNewTouch[totalNumElectrodes];
bool isNewRelease[totalNumElectrodes];
int numTouches = 0;

void setup(){  
  Serial.begin(57600);
  
  pinMode(LED_BUILTIN, OUTPUT);
   
  //while (!Serial) {}; //uncomment when using the serial monitor 
  Serial.println("Bare Conductive Multi Board Touch MP3 player");

  if(!MPR121.begin(MPR121_ADDR)) Serial.println("error setting up MPR121");
  MPR121.setInterruptPin(MPR121_INT);

  for(int i=0; i<12; i++){
    MPR121.setTouchThreshold(i, 12);
    MPR121.setTouchThreshold(i, 6);
  }
  
  for(int i=0; i<numSecondaryBoards; i++){
    for(int j=0; j<12; j++){
      thisExternalTouchStatus[i][j] = false;
      lastExternalTouchStatus[i][j] = false;
    }
  }

  for(int i=0; i<totalNumElectrodes; i++){
    isNewTouch[i] = false;
    isNewRelease[i] = false;
  }   

  for(int a=A0; a<=A5; a++){
    pinMode(a, OUTPUT);
    digitalWrite(a, LOW); 
  }

  Serial1.begin(9600);
  delay(100);

}

void loop(){
  
  // reset everything  that we combine from the two boards
  resetCompoundVariables();
  
  readLocalTouchInputs();
  
  readRemoteTouchInputs();
  
  processTouchInputs();
  
}


void readLocalTouchInputs(){

  // update our compound data on the local touch status

  if(MPR121.touchStatusChanged()){
    MPR121.updateTouchData();
    touchStatusChanged = true;
    
    for(int i=0; i<12; i++){
      isNewTouch[i] = MPR121.isNewTouch(i);
      isNewRelease[i] = MPR121.isNewRelease(i);
    }
  }
  numTouches+=MPR121.getNumTouches();

}

void readRemoteTouchInputs(){

  char incoming;

  for(int a=A0; a<A0+numSecondaryBoards; a++){

    digitalWrite(a, HIGH);
    delay(15);

    // only process if we have a full packet available
    while(Serial1.available() >= serialPacketSize){

      // save last status to detect touch / release edges
      for(int i=0; i<12; i++){
        lastExternalTouchStatus[a-A0][i] = thisExternalTouchStatus[a-A0][i];
      }
      
      incoming = Serial1.read();
      if(incoming == 'T'){ // ensure we are synced with the packet 'header'
        for(int i=0; i<12; i++){
          if(!Serial1.available()){
            return; // shouldn't get here, but covers us if we run out of data
          } else {
            if(Serial1.read()=='1'){
              thisExternalTouchStatus[a-A0][i] = true;
            } else {
              thisExternalTouchStatus[a-A0][i] = false;
            }
          }
        }
      } 
    }

    // now that we have read the remote touch data, merge it with the local data
    for(int i=0; i<12; i++){
      if(lastExternalTouchStatus[a-A0][i] != thisExternalTouchStatus[a-A0][i]){
        touchStatusChanged = true;
        if(thisExternalTouchStatus[a-A0][i]){
          // shift remote data up the array by 12 so as not to overwrite local data
          isNewTouch[i+(12*((a-A0)+1))] = true;
        } else {
          isNewRelease[i+(12*((a-A0)+1))] = true;
        }
      }

      // add any new touches to the touch count
      if(thisExternalTouchStatus[a-A0][i]){
        numTouches++;
      }
    }

    digitalWrite(a, LOW);
  }
}

void processTouchInputs(){
  // only make an action if we have one or fewer pins touched
  // ignore multiple touches
  
  //if(numTouches <= 1){
    for (int i=0; i < totalNumElectrodes; i++){  // Check which electrodes were pressed
      if(isNewTouch[i]){   
        //pin i was just touched
        
        Serial.print("pin ");
        Serial.print(i);
        Serial.println(" was just touched");
        
        Keyboard.press(keyMap[i]);
        digitalWrite(LED_BUILTIN, HIGH);        
      }
      else{
        if(isNewRelease[i]){
          Keyboard.release(keyMap[i]);
          /*
          Serial.print("pin ");
          Serial.print(i);
          Serial.println(" is no longer being touched");
          */
          digitalWrite(LED_BUILTIN, LOW);
       } 
      }
      
    }
 // }
}


void resetCompoundVariables(){

  // simple reset for all coumpound variables

  touchStatusChanged = false;
  numTouches = 0;

  for(int i=0; i<totalNumElectrodes; i++){
    isNewTouch[i] = false;
    isNewRelease[i] = false;
  }  
}

