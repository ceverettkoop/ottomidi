//Custom midi based synth with AY38910

#include "aycontrol.h"
#include <MIDI.h>

static unsigned long lastUpdate = 0;

short int currentChannelIn = 0;
float attackSpeed = 2.5;
float decaySpeed = 1;
float releaseSpeed = 1.5;

MIDI_CREATE_DEFAULT_INSTANCE();

struct channelStruct{
  short int currentPitch;
  short int currentAmp;
  short int finalAmp;
  short int oldAmp;
  unsigned int sinceOn;
  unsigned long timeOff;
  bool keyIsOn;
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
    pinMode(9,OUTPUT);


  //reset pin
    pinMode(2,OUTPUT);
    digitalWrite(2,HIGH);
    digitalWrite(2,LOW);
    delayMicroseconds(5); // more than 500us required.
    digitalWrite(2,HIGH);

    init2MhzClock();

    set_mix( true, true, true, false, false, false );

    set_envelope(1, 1, 0, 0, 5000);

    note_chA(50);
    note_chB(55);
    note_chC(57);

    // Connect the handleNoteOn function to the library,
    // so it is called upon reception of a NoteOn.
    MIDI.setHandleNoteOn(handleNoteOn);  // Put only the name of the function

    // Do the same for NoteOffs
    MIDI.setHandleNoteOff(handleNoteOff);

    // Initiate MIDI communications, listen to all channels
    MIDI.begin(MIDI_CHANNEL_OMNI);
}


void loop(){

  //read MIDI constantly
  MIDI.read();


  //update note status every 10ms
  unsigned long now = millis();
  if ( (now - lastUpdate) > 10 ) {

    lastUpdate += 10;


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

      // if amplitude was changed, send the change
      if ( (channelStatus[i].currentAmp / 4) != (channelStatus[i].oldAmp /4 ) ){
        updateAmp(i);

      }

      //advance timer
        channelStatus[i].sinceOn++;

    }

  }

}


int getAmplitude(struct channelStruct channel){

  short int attackLimit = (100 / attackSpeed) ;
  short int decayLimit = attackLimit * 1.5;

  //attack
  if (channel.sinceOn < attackLimit){
    return (channel.sinceOn * attackSpeed);
  }

  //decay
  if (channel.sinceOn < decayLimit){
    short int sinceDecay = channel.sinceOn - attackLimit;
    return  100 - (sinceDecay * decaySpeed);
  }

  if (channel.sinceOn >= decayLimit){
    //once you are past decayLimit we are in sustain mode unless the key is off
    channel.finalAmp = channel.oldAmp;

    if (channel.keyIsOn){
      return channel.finalAmp;
    }

    int elapsed = ( (millis() - channel.timeOff) / 1000);
    return (channel.finalAmp - (elapsed * releaseSpeed) );
  }

return 0;

}



void handleNoteOn(byte channel, byte pitch, byte velocity)
{

currentChannelIn++;
if (currentChannelIn == 3){
  currentChannelIn = 0;
}

//attackSpeed = (velocity / 128);

  channelStatus[currentChannelIn].sinceOn = 0;
  channelStatus[currentChannelIn].keyIsOn = true;

  digitalWrite(9,HIGH);

  switch (currentChannelIn){
    case 0: note_chA(pitch);
      break;
    case 1: note_chB(pitch);
      break;
    case 2: note_chC(pitch);
      break;
  }


}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  channelStatus[currentChannelIn].keyIsOn = false;
  channelStatus[currentChannelIn].timeOff = millis();
  digitalWrite(9,LOW);
}

void updateAmp(int channel){
  switch(channel){
    case 0:
      set_chA_amplitude((channelStatus[channel].currentAmp / 8), 0);
      break;
    case 1:
      set_chB_amplitude((channelStatus[channel].currentAmp / 8), 0);
      break;
    case 2:
      set_chC_amplitude((channelStatus[channel].currentAmp / 8), 0);
      break;

    }

}
