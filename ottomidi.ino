//Custom midi based synth with AY38910

#include "aycontrol.h"

static unsigned long lastUpdate = 0;

short int currentChannelIn = 0; // 0 = A, 1 = B, 2 = C

short int attackLimit = 100; //bigger number = longer
short int decayLimit = attackLimit + 20;
float attackSpeed = 3; // bigger number = faster
float decaySpeed = 2;
float releaseSpeed = .25;
bool buttonOn = 0;

struct channelStruct{
  short int currentPitch;
  short int currentAmp;
  short int finalAmp;
  short int oldAmp;
  unsigned int sinceOn;
  unsigned int sinceOff;
  bool isCurrent;
};

struct channelStruct channelStatus[]= {
  {50, 0, 0, 0, 0, 0, false},  //channel A
  {55, 0, 0, 0, 0, 0, false},  //channel B
  {57, 0, 0, 0, 0, 0, false}}; //channel C




void setup() {

   //init pins
    pinMode(latchPin, OUTPUT);
    pinMode(dataPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(pinBC1, OUTPUT);
    pinMode(pinBCDIR, OUTPUT);
    pinMode(freqOutputPin, OUTPUT);

    pinMode(3,INPUT); //red button for note trigger


  //reset pin
    pinMode(2,OUTPUT);
    digitalWrite(2,HIGH);
    digitalWrite(2,LOW);
    delayMicroseconds(5); // more than 500us required.
    digitalWrite(2,HIGH);

    init2MhzClock();

    set_mix( true, true, true, false, false, false );

    Serial.begin(9600);

    set_envelope(1, 1, 0, 0, 5000);

    note_chA(50);
    note_chB(55);
    note_chC(57);
}


void loop(){

  //update ever 10ms
  unsigned long now = millis();
  if ( (now - lastUpdate) > 10 ) {

    short int currentChannelIn = getCurrentChannel();
    getButton(currentChannelIn);
    lastUpdate += 10;
//    Serial.println(channelStatus[currentChannelIn].sinceOn);

    //set current channel in
    channelStatus[0].isCurrent = false;
    channelStatus[1].isCurrent = false;
    channelStatus[2].isCurrent = false;
    channelStatus[currentChannelIn].isCurrent = true;


    //set Amplitude for each channel
    for (int i = 0; i < 3; i++){

      channelStatus[i].oldAmp = channelStatus[i].currentAmp;

      channelStatus[i].currentAmp = getAmplitude(channelStatus[i]);

      if (channelStatus[i].currentAmp < 1){
        channelStatus[i].currentAmp = 0;
      }

      if (channelStatus[i].currentAmp > 100){
        channelStatus[i].currentAmp = 100;
      }

      channelStatus[i].sinceOn++;
      channelStatus[i].sinceOff++;
    }

    // if any amplitude was changed, send the change
    if ( (channelStatus[0].currentAmp / 8) != (channelStatus[0].oldAmp /8) ){
      Serial.println("changing channel A amp");
      set_chA_amplitude((channelStatus[0].currentAmp  / 8),0); //max volume 13?
    }

    // if any amplitude was changed, send the change
    if ( (channelStatus[1].currentAmp / 8) != (channelStatus[1].oldAmp /8) ){
      Serial.println("changing channel B amp");
      set_chB_amplitude((channelStatus[1].currentAmp / 8),0); //max volume 13?
    }

    // if any amplitude was changed, send the change
    if ( (channelStatus[2].currentAmp / 8) != (channelStatus[2].oldAmp /8) ){
      Serial.println("changing channel C amp");
      set_chC_amplitude((channelStatus[2].currentAmp / 8),0); //max volume 13?
    }

  }
}

void getButton(int channel){

  if (buttonOn == 0){ //if note has been off and button is pressed, note is ON
    if (digitalRead(3)){
      buttonOn = 1;
      channelStatus[channel].sinceOn = 0;
      channelStatus[channel].sinceOff = 1000;
  //    Serial.println("button");
    }
  }

  else{ //if has been on and is note off, new trigger OFF
    if (!digitalRead(3)){
      buttonOn = 0;
      channelStatus[channel].sinceOff = 0;
    }
  }
}

int getAmplitude(struct channelStruct channel){

  //attack
  if (channel.sinceOn < attackLimit){
  //  Serial.println("attack");
    return (channel.sinceOn * attackSpeed);
  }

  //decay
  if (channel.sinceOn >= attackLimit && channel.sinceOn < decayLimit){
    short int sinceDecay = channel.sinceOn - attackLimit;
//    Serial.println("decay");
    return  100 - (sinceDecay * decaySpeed);
  }

  //once you are past decayLimit we are in sustain mode until off
  //once we hit trigger off take snapshot of volume

  if (buttonOn && channel.sinceOn >= decayLimit){
    channel.finalAmp = channel.oldAmp;
    return channel.finalAmp;
//    Serial.println("sustain");
  }

  // release if button is done or if button is for other channel
  if (!buttonOn && channel.sinceOn >= decayLimit){
  //  Serial.println("release");
    return (channel.finalAmp - (channel.sinceOff * releaseSpeed));
  }

}

short int getCurrentChannel(){
  if (channelStatus[0].currentAmp == 0){
    return 0;
  }

  if (channelStatus[1].currentAmp == 0){
    return 1;
  }

  if (channelStatus[2].currentAmp == 0){
    return 2;
  }

  return 0;

}
