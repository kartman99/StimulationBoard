#include "serialCom.h"
//#include <pinout.h>

unsigned int nInterval = 3;
unsigned int nRow = 6;
unsigned int iRow = 0;
char deviceName[]="LightStimulator";
char deviceVersion[]="30";
char firmwareVersion[] = "0.1";

//Boards Produced:
// 1: eStim 
// 2: eStim inverted               Stimulationsboard_Arduino_V5.7 UV an PB0
// 3: light Stim with micro switch Stimulationboard_Light_V3.0    UV an PD4
// 4: light Stim with Trigger      Stimulationboard_Light_trig    UV an PD4
// 5: light Stim with Trigger      Landscape Format, 6 Trig Groups
// 6: light Stim with Trigger      Landscape Format, MyrVersion Trigger on pin 20


unsigned long uvFlash=8000; //uv Flash Duration in us
unsigned long long now;
unsigned long long last;
unsigned long long uvTimer;
unsigned long long iterationCounter;
unsigned long long iterationTimer;
unsigned long long milli_to_micro = 1000;
unsigned long long sec_to_micro = 1000*milli_to_micro;
float micro_to_milli = 0.001;

unsigned long long ul_tmp=0.0;
/* 
 *                 set Default Stimulation 
 */
unsigned long long stimInterval[]   = {1000000, 1000000,  500000,  500000,  333333,  333333}; // Time period between Stimulations in us
unsigned long long pulseDuration[]  = {   2000,    2000,    2000,    2000,    2000,    2000}; // Flash duration in us
unsigned long long pulseInterval[]  = {   4000,    4000,    4000,    4000,    4000,    4000}; // Flash interval in us


bool          bPulseOn[] = {   0,    0,    0,    0,    0,    0}; // 
bool          bActive[]  = {   1,    1,    1,    1,    1,    1}; // mark active rows
unsigned int  iFlash[]   = {   0,    0,    0,    0,    0,    0}; // Number of flashes per stimulation
unsigned int  nFlash[]   = {   5,    5,    5,    5,    5,    5}; // Number of flashes per stimulation

unsigned long long stimTimer[]      = {   1,    1,    1,    1,    1,    1};
unsigned long long pulseOnTimer[]   = {   1,    1,    1,    1,    1,    1};
unsigned long long pulseOffTimer[]  = {   1,    1,    1,    1,    1,    1};



           //ROW                           1          2          3          4          5          6                     
           //NAMES Pos                    PD6        PB4        PB6        PC7        PF6        PF4             
unsigned char pinPos[]             = { 0b01000000,0b00010000,0b01000000,0b10000000,0b01000000,0b00010000};
volatile unsigned char * portPos[] = {  &PORTD,     &PORTB,    &PORTB,    &PORTC,    &PORTF,    &PORTF};
volatile unsigned char *  dirPos[] = {  &DDRD,      &DDRB,     &DDRB,     &DDRC,     &DDRF,     &DDRF};
//                                       PB0
unsigned char pinTrig              = 0b00000100;    // ursprünglich 0b00000001
unsigned char intTrig              =    2;          //trigger interrupt ursprünglich auf 0
volatile unsigned char *  portTrig =   &PORTD;
volatile unsigned char *   dirTrig =    &DDRD;

//                                       PD4
unsigned char pinUV                = 0b00010000;
volatile unsigned char *  portUV   =   &PORTD;
volatile unsigned char *  dirUV    =    &DDRD;


/*
*      Set Defaults
*/

int bLight      = false;
int bFlash      = false;
int bStimulate  = true;
int bPermanent  = false;
int bSequential = false;
int bExtTrig    = false;

int trigRate = 25;   //trigger Rate in ms   50ms = 20Hz


// Tissues should not be lit for more that ~3mins
// If longe exposure is needed set bUvAuto=false
// or uvDuration to whatever value
int bUvAuto      = 1;
unsigned long uvDuration     = 180;       //turn UV Off after 180 sec
unsigned long long uvOffTime = 0;

char sbuf[50];
unsigned int dd;

int stimPinOffState=HIGH;
//int uvPinOffState=LOW;

void lightOn(void);
//void lightOff(void);

int trigPin = 2;  //PD1=D2
/******************************************************************/
/******************************************************************/
void setTimer() {  
  Serial.println("set Timer");
  stimTimer[0]=now;
  if (bSequential){
    for (iRow = 1; iRow < nRow; iRow++)
       stimTimer[iRow] = stimTimer[iRow-1]+nFlash[iRow-1]*pulseInterval[iRow];
  } else {
    for (iRow = 1; iRow < nRow; iRow++)
       stimTimer[iRow] = stimTimer[0];    
  }
  //
  for (iRow = 0; iRow < nRow; iRow++){
    pulseOnTimer[iRow] =stimTimer[iRow];    
    pulseOffTimer[iRow]=pulseOnTimer[iRow]+pulseDuration[iRow];    
  }
}
/******************************************************************/
/******************************************************************/
void setup() {
  TXLED0;
  RXLED0;  
  // Higher speeds do not appear to be reliable
  now=micros();
  Serial.begin(9600);
  bSequential=0;      
  
  // Tims alter Code
  //set pin input
  *dirTrig  &=  ~pinTrig;
  //set trigger pin high (Pullup)
  *portTrig |= pinTrig;  // (DDxn, PORTxn = 0b01) = intermediate state with pull-up enabled
  // set Interrupt 
  attachInterrupt(intTrig, getTrigger, CHANGE);   // getTrigger ist die isr()
  
  // set UV pin out
  *dirUV   |= pinUV;
  // set stimulation pins
  for (int i=0; i< nRow;i++)
    *dirPos[i] |= pinPos[i];
  
  uvTimer  = now;
  lightOff();
  setTimer();
  iterationTimer=now;
  iterationCounter=0;  
}
/******************************************************************/
/******************************************************************/
void getTrigger() {
  // Serial.print('t');
  if(PIND & (1<<2)) {          //       -- Hiermit ging es
//  if (digitalRead(trigPin)){ //RISING -- Hiermit ging es irgendwie nicht
      if (bFlash){
        sendUvFlash();
      }
  } else { //FALLING      
      if (bStimulate) {
        startPulse();
      }
  }  
}
/******************************************************************/
/******************************************************************/
void sendUvFlash(){
  lightOn();
  delayMicroseconds(uvFlash);
  //delay(uvFlash);
  lightOff();
}
/******************************************************************/
/******************************************************************/
void stimSwitchOn(void) {  
  for (int iRow=0; iRow < nRow; iRow++) {
      if (bActive[iRow]){
        *portPos[iRow] &= ~pinPos[iRow]; // on
      }      
  }  
}
/******************************************************************/
/******************************************************************/
void stimSwitchOff(void) {  
  for (int iRow=0; iRow < nRow; iRow++) {
      if (bActive[iRow]){
        *portPos[iRow]|=pinPos[iRow]; // off  
      }
  }  
}
/******************************************************************/
/******************************************************************/
void setStimulation(uint64_t rr){
  for (int iRow=0; iRow < nRow; iRow++) {
      stimInterval[iRow]=rr;
  }  
}
/******************************************************************/
/******************************************************************/
void setPulseDuration(long long pd){
  for (int iRow=0; iRow < nRow; iRow++) {
      pulseDuration[iRow]=pd;
  }  
}
/******************************************************************/
/******************************************************************/
void setPulseNumber(int n){
  for (iRow=0; iRow < nRow; iRow++) {
    nFlash[iRow] = n;
  }
}
/******************************************************************/
/******************************************************************/
void lightOn(void) {  
  *portUV &= ~pinUV;  
}
/******************************************************************/
/******************************************************************/
void lightOff(void) {
  *portUV |= pinUV;
}
/******************************************************************/
/******************************************************************/
void lightSwitchOn(void) {  
  uvOffTime = now + uvDuration * sec_to_micro;
  lightOn();
  bLight=1;
}
/******************************************************************/
/******************************************************************/
void lightSwitchOff(void) {  
  bLight=0;
  lightOff();
}
/******************************************************************/
/******************************************************************/
void startPulse(){
    //turn lights on
    for (int iRow=0; iRow < nRow; iRow++) {
      if (! bActive[iRow])
         continue;
      if (now < stimTimer[iRow])
         continue;                  
      if (! bPulseOn[iRow] && now >= pulseOnTimer[iRow]){
        //start pulse        
        bPulseOn[iRow]=true;
        *portPos[iRow]&=~pinPos[iRow]; // on        //digitalWrite (pinPos[iRow], LOW);
        pulseOffTimer[iRow]= now + pulseDuration[iRow];
        pulseOnTimer[iRow] = now + pulseInterval[iRow];
      }
  }  // end rows
}
/******************************************************************/
/******************************************************************/
void stopPulse(){
   //turn lights off
   for (int iRow=0; iRow < nRow; iRow++) {
      if (! bActive[iRow])
         continue;
      if (now < stimTimer[iRow])
         continue;                  
      if ( bPulseOn[iRow] && now >= pulseOffTimer[iRow]){
        //end pulse
        bPulseOn[iRow]=false;
       *portPos[iRow]|=pinPos[iRow]; // off //digitalWrite (pinPos[iRow], HIGH);
       iFlash[iRow]++;
      }
      //
      if (iFlash[iRow] == nFlash[iRow]){  //end of pulse train
        iFlash[iRow]=0;        
        stimTimer[iRow]   =stimTimer[iRow] + stimInterval[iRow];
        pulseOnTimer[iRow]=stimTimer[iRow];        
     }      
   }  // end rows
}
/******************************************************************/
/******************************************************************/
void printLongLong(uint64_t n){
  //Print unsigned long long integers (uint_64t)
  //CC (BY-NC) 2020
  //M. Eric Carr / paleotechnologist.net
  unsigned char temp;
  String result=""; //Start with a blank string+
  
  if(n==0){Serial.println(0);return;} //Catch the zero case
  while(n){
    temp = n % 10;
    result=String(temp)+result; //Add this digit to the left of the string
    n=(n-temp)/10;      
    }//while
  Serial.println(result);
}
/******************************************************************/
/******************************************************************/
void readSerial(MY_CMD cc){
  //         
  switch (cc.cCMD) {
      case  NULL:    // Could not read Command      
        Serial.println("Failed to read Command");
        break;
      case '0':   //48:    //'0'      
        Serial.println(deviceName);
        break;
      case 'v':   //48:    //'0'      
        Serial.println(deviceVersion);
        Serial.println(__FILE__);
        break;
      case 'V':
        Serial.println(firmwareVersion);
      case 'A': 
        //
        if (cc.nCount ==0 ){
          Serial.print("Active lines: ");
          for (iRow=0; iRow<nRow; iRow++)
               Serial.print(bActive[iRow]);
          Serial.println("");
        } else if (cc.nData[0]==0) {
          Serial.println("Set all lines Active: ");
          for (iRow=0; iRow<nRow; iRow++){
            bActive[iRow]=1;
          }
        }else {
          iRow=cc.nData[0]-1;
          Serial.print("Set line "); Serial.print(iRow+1); Serial.println(" Active");
          bActive[iRow]=1;
        }
        break;
      case 'a': 
        if (cc.nCount ==0 ){
          Serial.println("Set all lines inactive: ");
          for (iRow=0; iRow<nRow; iRow++){
            bActive[iRow]=1;
            stopPulse();
            bActive[iRow]=0;
          }
        } else {
          iRow=cc.nData[0]-1;
          Serial.print("Set line "); Serial.print(iRow+1);Serial.println(" inactive");          
          stopPulse();
          bActive[iRow]=0;
        }        
        break;
      case 'f' :
        Serial.println("Flash UV Light Off");
        lightSwitchOff();
        bFlash=false;
        break;
      case 'F':        
        Serial.println("Flash UV Light On");
        if (cc.nCount >0 ){
           Serial.print("uvFlash Duration to");
           Serial.println(cc.nData[0]);
           uvFlash=cc.nData[0];
        }
        bFlash=true;
        lightSwitchOff();
        //restart internal trigger
        break;
      case 'I':        
        Serial.print("IterationCounter: ");
        printLongLong(iterationCounter);
        Serial.print(" IterationTimer: ");
        printLongLong(iterationTimer);
        Serial.print(" ms, Time per Iteration: ");
        Serial.print(1000.0*(now-iterationTimer)/iterationCounter);
        Serial.println("  µs per iteration");
        iterationCounter=0;
        iterationTimer=now;
        break;  
      case 'm':        
        //return now to calculate timer offset
        //Serial.println(now);
        break;
      case 'n':        
        Serial.println("UV AutoOff Off");
        //bLight=0;
        bUvAuto=false;
        break;
      case 'N':                     //can be used for tetanic Stimulation 200*50fmp=4 sec tetanus
         //unsigned long long pulseDuration[]  = {   2000,    2000,    2000,    2000,    2000,    2000,    2000,    2000}; // Flash duration in us
         //unsigned long long pulseInterval[]  = {   4000,    4000,    4000,    4000,    4000,    4000,    4000,    4000}; // Flash interval in us
         //unsigned int  nFlash[]              = {   5,    5,    5,    5,    5,    5,    5,    5}; // Number of flashes per stimulation
        if (cc.nCount ==0 ){
            Serial.print("Pulse Number is: ");
            Serial.print(nFlash[0]);
            Serial.print(" Pulse Duration is: ");
            Serial.print(micro_to_milli * pulseDuration[0]);
            Serial.print(" ms, Pulse Interval is: ");
            Serial.print(micro_to_milli * pulseInterval[0]);
            Serial.println(" ms");    
        } else {
          if (cc.nCount >0 ){
            Serial.print("Set Pulse Number from ");
            Serial.print(nFlash[0]);
            Serial.print(" to ");
            Serial.println(cc.nData[0]);
            for (int iRow=0; iRow < nRow; iRow++)
                   nFlash[iRow]=cc.nData[0];
          }
          if (cc.nCount >1 ){
            ul_tmp = milli_to_micro * cc.nData[1];
            Serial.print("Set Pulse Duration from ");
            Serial.print((float) pulseDuration[0]);
            Serial.print(" us to ");
            Serial.print( (float) ul_tmp);
            Serial.println(" us");
            for (int iRow=0; iRow < nRow; iRow++)
                   pulseDuration[iRow]=ul_tmp;
          }
          if (cc.nCount >2 ){
            ul_tmp = milli_to_micro*cc.nData[2];
            Serial.print("Set Pulse Interval from ");
            Serial.print((float) pulseInterval[0]);
            Serial.print(" us to ");
            Serial.print((float) ul_tmp);
            Serial.println(" us");
            for (int iRow=0; iRow < nRow; iRow++)
                   pulseInterval[iRow]=ul_tmp;
          }
        }
        break;                    
      case 'p' :
        Serial.println("Permanent Stim Off");
        bPermanent=0;
        bStimulate=0;          
        //stimSwitchOff();
        break;
      case 'P':        
        Serial.println("Permanent Stim On");
        bStimulate=1;
        bPermanent=1;
        if (cc.nCount >1 ){
          
        }
        //stimSwitchOn();
        Serial.println("Permanent On");
        break;
      case 'Q':
        Serial.print("Set Stimulation to seQuential");
        bSequential=true;
        setTimer();
        break;
      case 'q':
        Serial.print("SetStimulation to parallel");
        bSequential=false;
        setTimer();
        break;        
      case 'R':              
        if (cc.nCount >0 ){
            Serial.print("Set only Row "); Serial.print(cc.nData[0]);
            for (iRow=0; iRow<nRow; iRow++)
               bActive[iRow]=0;
            bActive[cc.nData[0]]=1;
            
        } else {
          Serial.print("Set Rows 1-8 ");
          for (iRow=0; iRow<nRow; iRow++)
               bActive[iRow]=1;           
        }
        Serial.println(" active");
        break;
      case 's' :
        Serial.println("Stimulation Off");
        bStimulate=false;
        bPermanent=false;
        stimSwitchOff();
        break;
      case 'S':
        //
        if (cc.nCount ==1) {
           //unsigned long long stimInterval[]   = {1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000}; // Time period between Stimulations in us
            ul_tmp = milli_to_micro*cc.nData[0];
            Serial.print("Set Stimulation Interval (RR) to ");
            Serial.print((float) ul_tmp);
            Serial.println(" us");
            for (int iRow=0; iRow < nRow; iRow++)
                   stimInterval[iRow]=ul_tmp;
        } else if (cc.nCount ==2) {
            ul_tmp = milli_to_micro*cc.nData[0];
            iRow   = cc.nData[1]-1;
            Serial.print("Set Stimulation Interval (RR) if Row ");
            Serial.print(iRow+1);
            Serial.print(" to ");
            Serial.print((float) ul_tmp);
            Serial.println(" us");            
            stimInterval[iRow]=ul_tmp;            
        }
        Serial.println("Stimulation On, Interval (RR) is:");
        for (iRow=0; iRow<nRow; iRow++)
              printLongLong(stimInterval[iRow]);
        //      
        bStimulate=true;
        bPermanent=false;
        //restart internal trigger
        setTimer();
        //stimSwitchOn();
        Serial.print("bSequential "); Serial.println(bSequential);
        break;                    
      case 't' :
        Serial.println("External Trigger Off");
        bExtTrig=false;
        //use internal Trigger
        break;
      case 'T':        
        Serial.print("External Trigger On auf pin  ");
        Serial.println(trigPin);
        bExtTrig=true;
        break;
      case 'u' :        
        Serial.println("UV Light Off");
        lightSwitchOff();
        bFlash=false;
        break;
      case 'U':        
        bFlash=false;
        Serial.print("Permanent UV Light On, ");        
        //
        bExtTrig=false;
        //
        if (cc.nCount >0 ){
           Serial.println(" set AutoOff, ");
           if ( cc.nData[0] == 0 ){              
              bUvAuto=false;
           } else {
              bUvAuto=true;
              uvDuration=cc.nData[0];              
           }
        }
        
        if (bUvAuto){
          Serial.print("AutoOff in ");
          Serial.println( uvDuration );
        } else {
          Serial.println("AutoOff is off");
        }
        //
        bFlash=false;
        //
        lightSwitchOn();
        //
        break;
      case 'W':        
        if (cc.nCount >0 ){
            Serial.print("Set Puls width to "); Serial.print(cc.nData[0]);
            for (iRow=0; iRow<nRow; iRow++)
               pulseDuration[iRow]=cc.nData[0]*milli_to_micro;
        } else {
          Serial.println("Puls width is "); 
          for (iRow=0; iRow<nRow; iRow++)
            Serial.println( int(micro_to_milli * pulseDuration[iRow]));
        }
        break;      
      default:;
  }
}
/******************************************************************/
/******************************************************************/
void loop() {
  //  
  last=now;
  now=micros();
  // work-around for micros overflow issue (every 70mins)
  if (last > now)
    setTimer();  
  //
  iterationCounter++;
  //
  if (bUvAuto && now > uvOffTime)
    lightSwitchOff();
  //
  if (bLight) {
    lightOn();
  } else {
    lightOff();
  }
  //
  if (! bExtTrig  && bStimulate)
      startPulse();
  // in case pulse start is triggered externally check if is due stopping
  if (! bPermanent)
      stopPulse();
  //
  if (Serial.available() > 0){
    MY_CMD cc;
    myGET_CMD(cc);
    readSerial(cc);
  }  
}
