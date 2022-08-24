/* IMPORTANT: USE ARDUINO PIN 3 FOR PWM INPUT. Also make sure your input is protected by at least a resistor, 100kohm, if its a 5v pwm signal. If 12v you may want a level shifter
or at minimum a voltage divider.

MAKE SURE TO SET OUTPUT MODE ON HALTECH TO DUTY CYCLE.
*/

#include "mcp_can.h"
#include <SPI.h>


MCP_CAN CAN(9); //default for those can bus shields 
#define CAN_INT 2 //default for those can bus shields 
char msgString[128];//terrible way of comparing data
long unsigned int rxId;//ID of incoming message
long unsigned int hi = 0x1B200002;
unsigned char len = 0;//not used here, since we dont care about contents of message
unsigned char rxBuf[8];//not used here, since we dont care about contents of message
unsigned char keepAlive[8] = {0x00, 0x00, 0x22, 0xE0, 0x41, 0x90, 0x00, 0x00};//counter KA
unsigned char speed[8] = {0xBB, 0x00, 0x3F, 0xFF, 0x06, 0xE0, 0x00, 0x00};//counter KA
unsigned char slowRollTable[4] = { 0x00, 0x40, 0x80, 0xC0};
unsigned long previousMillis = 0;
const long interval = 15;//how fast we send
int slowCounter; //Which number in array we at
int slowRoll;
int roll;//how many data messages we're sent, 0-30
volatile unsigned long int pwm_value = 0;
volatile unsigned long int prev_time = 0;
bool oHi = false;
unsigned long calVel;

void rising() {
  attachInterrupt(1, falling, FALLING);
  prev_time = micros();
   
}
 
void falling() {
  attachInterrupt(1, rising, RISING);
  pwm_value = micros()-prev_time;

  calVel = map(pwm_value, 0, 4600, 6000, 0);//THIS IS ASSUMING 200HZ

  if (calVel > 6000)//Due to Arduino uno being 8 bit, this causes a quick overflow so we do this ugly little patch
  {
    calVel = 0;
  }
    /*USE THESE ONLY FOR WHEN YOUR OUTPUT FREQUENCY IS NOT 200hz. Most haltechs are 200-500, which god knows which one will work. If any issues, contact me.
  Serial.print("pwm ");
  Serial.println(pwm_value);
  Serial.print("mapped ");
  Serial.println(calVel);*/
}

void setup()
{
  
  Serial.begin(115200);
  attachInterrupt(1, rising, RISING);//Uno Pin 3 for input
  if(CAN.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) Serial.print("CAN init ok\r\n");
  else Serial.print("CAN init failed\r\n");
  CAN.setMode(MCP_NORMAL);
  pinMode(CAN_INT, INPUT);

}
 
void loop()
{

  unsigned long currentMillis = millis();
speed[6] = calVel / 256;
speed[7] = calVel;
  
   if (oHi == true){
       if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
            if (roll == 0)
            { 
              CAN.sendMsgBuf(0x1AE0092c, 1, 8, keepAlive);//send keep alive with roll
              keepAlive[0] = slowRollTable[slowRoll];
              slowRoll++;
              if (slowRoll == 4){slowRoll = 0;}//reset slowroll
            } 

              CAN.sendMsgBuf(0x2104136, 1, 8, speed); //send speed
              roll++;
              if (roll == 30){roll = 0;}//reset roll
            }
   }
  

  if (!digitalRead(CAN_INT))//Wait for some Can data to come in
  {
    CAN.readMsgBuf(&rxId, &len, rxBuf);
   
        if ((rxId & 0x80000000) == 0x80000000)
      sprintf(msgString, "%.8lX", (rxId & 0x1FFFFFFF));
      
    if (!strcmp(msgString, "1B200002")){//This is a workaround since extID is 29 bit and arduino is 8 bit.

          oHi = true;
        }
 
  }


}