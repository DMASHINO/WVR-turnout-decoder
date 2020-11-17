

/* This is the latest code written Oct 2020 by Dave Mashino
 * Added control for LED panel lights, Twisted pair BRN=Neg, OR/WT = Green LED, OR = Red
 * Fixed on board LED flash when loconet switch command is rec'd
 * Normal servo operation is closed = CCW looking into shaft
 */
#include <NmraDcc.h>
#include <Servo.h>
#include <EEPROM.h>
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Defines and structures
//
// Default NmraDCC code from example
NmraDcc  Dcc ;

// *** change these variables as required
const int switchAddress = 1
; // decoder address of this turnout
const int servoFullCW = 20; // Thrown position if servoRev = 0
const int servoFullCCW = 100;  // Closed position if servoRev = 0
int servoRev = 0; // normal = 0, reversed = 1
int revPoints = 1; // reverses the polarity on the switch points, normal = 0, rev = 1
// ***
long servoDelay = 60; // servo speed, higher number is slower
long blinkDelay = 200; // LED blink timing, higher number is slower
// Define Arduino pins
const int fireRelay = 6;
const int RedLED = 9;
const int GrnLED = 8;
const int servoPwr = 12;
const int signalLED = 13;
// Declare General variables
int switchStat;
long blinkTime = 0;
int blinkOn = 1;
int needToBlink = 0;
int currServoPos = 90; // move servo to center at startup
long servoTime = 0;
int servoMoving = 1;
int lastStatus;
byte epReadChar;
// Declare myservo1 as a servo
Servo myservo1;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void BlinkLed() // Causes the LED to blink as the servo moves
{
  if ((millis() - blinkDelay) > blinkTime) {
  //blink is complete, reverse the output and put blinkTime to 0
    if (blinkOn == 1) {
      if (switchStat){
        digitalWrite(GrnLED,LOW);
      }
      else {
        digitalWrite(RedLED,LOW);        
      }
      blinkOn=0;
    }
    else {
      if (switchStat){
        digitalWrite(GrnLED,HIGH);
      }
      else {
        digitalWrite(RedLED,HIGH);        
      }
      blinkOn=1;
    }
    blinkTime=millis();
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Move any servos that are changing location
void MoveServos()
{   
    if (servoMoving){ // if set, zero out and move servos a step if needed
      servoTime = millis(); 
      servoMoving = 0;
      
      if(switchStat){ // switch is closed or closing
        if (servoRev){
          if(currServoPos > servoFullCW ){
            currServoPos --;
          }
        }
        else{
          if(currServoPos < servoFullCCW){
            currServoPos ++;            
          }
        }
      }
      else { // switch is thrown or throwing
        if (servoRev){
          if(currServoPos < servoFullCCW ){
            currServoPos ++;
          }
        }
        else{
          if(currServoPos > servoFullCW){
            currServoPos --;            
          }
        }
      }
     myservo1.write(currServoPos);    
    } 
    else{
     if((millis() - servoTime) > servoDelay){
       servoMoving = 1;
     }
   }   
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Basic accessory packet handler 
//

// This function is called whenever a normal DCC Turnout Packet is received
void notifyDccAccTurnoutOutput( uint16_t Addr, uint8_t Direction, uint8_t OutputPower )
{
  int address = 0;
  if(switchAddress == Addr){
    digitalWrite(signalLED,HIGH);
    delay(100);
    digitalWrite(signalLED,LOW);
    boolean enable = (Direction & 0x01) ? 1 : 0;
    if( enable )
    {
      switchStat = 1;
    }else{
       switchStat = 0;
    }     
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Setup
//
void setup() { 
  
   // Call the main DCC Init function to enable the DCC Receiver
   Dcc.init( MAN_ID_DIY, 10, CV29_ACCESSORY_DECODER | CV29_OUTPUT_ADDRESS_MODE, 0 );

   myservo1.attach(10);  // attaches the servo on pin 10 to the servo object
   pinMode(fireRelay, OUTPUT);
   pinMode(RedLED,OUTPUT);
   pinMode(GrnLED,OUTPUT);
   pinMode(servoPwr,OUTPUT);
   pinMode(signalLED,OUTPUT);
   digitalWrite(servoPwr, LOW);
   switchStat = readeprom();
   if (servoRev){
     if(switchStat){
       currServoPos = servoFullCW;
     }
     else{
       currServoPos = servoFullCCW;       
     }
   }
   else{
     if(switchStat){
       currServoPos = servoFullCCW;
     }
     else{
       currServoPos = servoFullCW;       
     }
//   delay(1000);
   }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Main loop
//
void loop()
{
  int addr = 0;
  String stringVal;  
  if (switchStat){
    if (revPoints){
      digitalWrite(fireRelay,HIGH);             
    }
    else{
      digitalWrite(fireRelay,LOW);     
    }
  }
  else{
    if (revPoints){
      digitalWrite(fireRelay,LOW);             
    }
    else{
      digitalWrite(fireRelay,HIGH);     
    }
  }
  if(needToBlink){
    digitalWrite(servoPwr,HIGH);
  }
  else{
    digitalWrite(servoPwr,LOW);
  }
  MoveServos();
  
  Dcc.process();
  
  if(switchStat != lastStatus){    
  // Write new switchStat to the EPROM  
    EEPROM.write(addr,'%');
    addr++;
    stringVal = String(switchStat);
    Serial.println(stringVal);
    EEPROM.write(addr,stringVal.charAt(0));
    lastStatus = switchStat;
  }

// Check for servos not yet in posistion
  if (switchStat) {
    if (servoRev == 0 && currServoPos < servoFullCCW){
      needToBlink = 1;
    }
    else if (servoRev == 1 && currServoPos > servoFullCW){
      needToBlink = 1;
    }
        
    else {
      needToBlink = 0;
    }
  }
  else{
    if (servoRev == 0 && currServoPos > servoFullCW){
      needToBlink = 1;
    }
    else if (servoRev == 1 && currServoPos < servoFullCCW){
      needToBlink = 1;
    }
    
    else {
      needToBlink = 0;
    }

  }
  if (needToBlink) {
    BlinkLed();
  }
  else {
    if (switchStat) {
      digitalWrite(GrnLED,HIGH);
      digitalWrite(RedLED,LOW);
    }
    else {
      digitalWrite(GrnLED,LOW);
      digitalWrite(RedLED,HIGH);      
    }
  }

}
/////////////////////////////////////////////////////////////////////////

int readeprom(){ // reads the value for the last SwitchStat saved
  int addr = 0;
  char chvalue;
  String strValue;
  
  chvalue = EEPROM.read(addr); // read first char
  if (chvalue != '%'){ // didn't read % sign there is no data available
    return(-1);    
  }
  addr++;   
  chvalue = EEPROM.read(addr);  // read next character this is a 1 or a 0
  strValue += chvalue;
  Serial.println(strValue);  
  return(strValue.toInt());  
}
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
