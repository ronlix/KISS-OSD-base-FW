#include "VTXcontrol.h";

#define EEVer 100

#define PINE _SFR_IO8(0x0C)
#define PINE0 0
#define PINE1 1
#define PINE2 2
#define PINE3 3
#define PINE4 4
#define PINE5 5
#define PINE6 6

#define DDRE _SFR_IO8(0x0D)
#define DDE0 0
#define DDE1 1
#define DDE2 2
#define DDE3 3
#define DDE4 4
#define DDE5 5
#define DDE6 6
#define DDE7 7

#define PORTE _SFR_IO8(0x0E)
#define PORTE0 0
#define PORTE1 1
#define PORTE2 2
#define PORTE3 3
#define PORTE4 4
#define PORTE5 5
#define PORTE6 6
#define PORTE7 7

static uint16_t channelTable[40] = {
  0x510A,    0x5827,    0x5F84,    0x66A1,    0x6DBE,    0x751B,    0x7C38,    0x8395, //  RB
  0x7981,    0x758D,    0x7199,    0x6DA5,    0x69B1,    0x65BD,    0x6209,    0x5E15, //  A
  0x5F9D,    0x6338,    0x6713,    0x6AAE,    0x6E89,    0x7224,    0x75BF,    0x799A, //  B
  0x5A21,    0x562D,    0x5239,    0x4E85,    0x7D35,    0x8129,    0x851D,    0x8911, //  E
  0x610C,    0x6500,    0x68B4,    0x6CA8,    0x709C,    0x7490,    0x7884,    0x7C38  //  F 
};

static uint8_t usedChannel = 1;
static uint8_t usedBand = 5;

void showState(){
  PORTD &= ~(1<<5) & ~(1<<6) & ~(1<<7) & ~(1<<4) & ~(1<<3);
  PORTB &= ~(1<<0);
  PORTE &= ~(1<<2);
  PORTC &= ~(1<<0) & ~(1<<1) & ~(1<<2) & ~(1<<3) & ~(1<<4) & ~(1<<5);
  switch (usedChannel){
    case 1: PORTD |= (1<<5); break;
    case 2: PORTD |= (1<<6); break;
    case 3: PORTD |= (1<<7); break;
    case 4: PORTB |= (1<<0); break;
    case 5: PORTD |= (1<<4); break;
    case 6: PORTD |= (1<<3); break;
    case 7: PORTC |= (1<<3); break;
    case 8: PORTC |= (1<<0); break;
  }
  switch (usedBand){
    case 1: PORTC |= (1<<1); break;
    case 2: PORTC |= (1<<2); break;
    case 3: PORTE |= (1<<2); break;
    case 4: PORTC |= (1<<4); break;
    case 5: PORTC |= (1<<5); break;
  }
}


void InitVTX(){
  DDRC |= (1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5);
  DDRD |= (1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7);
  DDRB |= (1<<0);
  DDRE |= (1<<2);
  DDRB &= ~(1<<1);
  PORTB |= (1<<1);
  
  
  if(EEPROM.read(0) == 0 || EEPROM.read(0) != EEVer){
    EEPROM.write(0, EEVer);
    EEPROM.write(1, usedChannel);
    EEPROM.write(2, usedBand);
  }else{
    usedChannel = EEPROM.read(1);
    usedBand = EEPROM.read(2); 
  }
  delay(500);
  
  PCMSK0 |= (1<<1);
  PCICR = (1<<0);
  
  VTXsetChannel(((usedBand-1)<<2)|(usedChannel-1));
}

void channelUp(){
   if(++usedChannel == 9) usedChannel = 1;
   VTXsetChannel(((usedBand-1)<<2)|(usedChannel-1));
}


void BandUp(){
   if(++usedBand == 6) usedBand = 1;
   VTXsetChannel(((usedBand-1)<<2)|(usedChannel-1));
}


void checkChannel(){
  static uint32_t wasHigh = 0;
  static uint32_t wasLow = 0;
  if(wasArmed == 0){
    if(throttle > 75 && yaw > 75){
      wasHigh = millis();
    }else if(throttle > 75 && wasHigh !=0 && millis()-wasHigh < 1000 && millis()-wasHigh > 10){
      channelUp();
      wasHigh = 0;
    }else wasHigh = 0;
    if(throttle > 75 && yaw < -75){
      wasLow = millis();
    }else if(throttle > 75 && wasLow !=0 && millis()-wasLow < 1000 && millis()-wasLow > 10){
      BandUp();
      wasLow = 0; 
    }else wasLow = 0; 
  }
}

ISR(PCINT0_vect){
  static uint32_t wasLow;
  if(PINB & (1<<1)){
     if(millis()-wasLow > 20 && millis()-wasLow < 1000){
       channelUp();
     }else if(wasLow > 1000){
       BandUp();
     }
  }else wasLow = millis();
}



void VTXsetChannel(uint8_t chan){
  uint32_t ChannelValue;

  ChannelValue = (uint32_t)channelTable[chan] | 0x40000;
  VTXsendTransmission(0x00, 0x0190);
  VTXsendTransmission(0x01, ChannelValue);
  

  EEPROM.write(1, usedChannel);
  EEPROM.write(2, usedBand);  

  showState();
}

void VTXsendTransmission(uint8_t Adr, uint32_t Value){
  uint32_t WriteWord = (uint32_t)(((Value << 5) | 0x10) | Adr);
  SPItransferWord(WriteWord);
}

void SPItransferWord(uint32_t WordOut){
  uint8_t i;
  PORTC |= (1<<VTXssPIN);
  delayMicroseconds(1);	
  PORTC &= ~(1<<VTXclkPIN);
  delayMicroseconds(1);
  PORTC &= ~(1<<VTXssPIN);	
  delayMicroseconds(1);
  for (i = 0; i<32; i++){
    PORTC &= ~(1<<VTXclkPIN);
    delayMicroseconds(1);
    if((WordOut >> i)&0x01) PORTC |= (1<<VTXdataPIN);
    else PORTC &= ~(1<<VTXdataPIN);
    delayMicroseconds(1);
    PORTC |= (1<<VTXclkPIN);
    delayMicroseconds(1);
  }
  PORTC |= (1<<VTXssPIN);
  delayMicroseconds(1);
  PORTC &= ~(1<<VTXdataPIN);	
  delayMicroseconds(1);	
}



