#include <Arduino.h>
#include "CamCom.h"


byte ReadCount = 0;
byte SerialArray[200];
int ReadByte = 0x00;


void processIncoming(byte Arr[200]){
  byte mac_add[6], Response[6]{Preamble0, Preamble1, 0x31, 0x12, HiByte, LoByte};
  uint16_t uid = 0; // will hold incase it's a get uid command
  switch(Arr[3]){
    case 0x1A: // Requesting for UID : SerialArray = {0xAA, 0x55, 0x00, 0x01, 0xFF, 0x00}
      uid = mesh.getNodeId() % 100000;
      SerialArray[2] = 0x02; // Change the length to two since the UID is a data of size 2
      SerialArray[3] = 0x1B; // Change the command to response to UID request
      SerialArray[4] = highByte(uid);
      SerialArray[5] = lowByte(uid);
      SerialArray[6] = HiByte;
      SerialArray[7] = LoByte;
      vTaskDelay(100 / portTICK_PERIOD_MS);
      Serial.write(SerialArray, 8); // Send the UID
      printArray("Your ID", SerialArray, 8);
      break;
    case 0x10: // Check if connected to mesh
      SerialArray[2] = 0x01; // Change the length to two since the UID is a data of size 2
      SerialArray[3] = 0x11; // Change the command to response to UID request
      SerialArray[4] = ConnectedToNet;
      SerialArray[5] = HiByte;
      SerialArray[6] = LoByte;
      // SerialArray[7] = mesh.isConnected(3793620052);
      // vTaskDelay(10 / portTICK_PERIOD_MS);
      Serial.write(SerialArray, 7); // Send the UID
      printArray("Your connected status", SerialArray, 7);
      break;
    case 7: //It's a message, so just send
      if(sendMessage(SerialArray, ReadCount))
        MyPrinter_println("Send good");
      else
        MyPrinter_println("Send Bad");
    case 0x0F: //It's a message feedback, so just send
      sendMessage(SerialArray, ReadCount);
      break;
  }
}
    
void setup(){
  Serial.begin(115200);
  initCom();
}

void makeUpSerialArr(byte arr[100], byte len, byte cmd){
  SerialArray[0] = Preamble0; SerialArray[1] = Preamble1;//header
  SerialArray[2] = len;//length of the data
  SerialArray[3] = cmd;//the cmd
  SerialArray[4] = 1;//the instanceIndex 
  SerialArray[5] = 0;//the subindex
  SerialArray[6] = 0;//the meshOpcode
  SerialArray[7] = 0;//the meshOpcode
  //the data content
  for(byte i = 0; i < len; i ++)
    SerialArray[8 + i] = arr[i];
  SerialArray[len + 8] = HiByte;
  SerialArray[len + 9] = LoByte;
  ReadCount = len + 10;
}

void makeUpMessage(uint16_t Source, uint16_t Destination, byte arr[100], byte len, byte msg_cmd){ // add the source and destination, then the send msg cmd
  // Serial.print(".");
  byte msg_arr[100];
  msg_arr[0] = msg_cmd;
  msg_arr[1] = highByte(Destination);
  msg_arr[2] = lowByte(Destination);
  msg_arr[3] = highByte(Source);
  msg_arr[4] = lowByte(Source);
  for(byte i = 0; i < len; i ++)
    msg_arr[5 + i] = arr[i];
  makeUpSerialArr(msg_arr, len + 5, 7);
}

void makeUpCommand(byte cmd_arr[200]){
  // makeUpMessage(MyUID, TerminalUID, cmd_arr, 2);
}

void loop(){
  mesh.update();
  if(Serial.available()){
    vTaskDelay(500 / portTICK_PERIOD_MS);
    ReadCount = 0;
    while(Serial.available()){
      ReadByte = Serial.read();
      SerialArray[ReadCount] = ReadByte;
      ReadCount += 1;
    }
    if(ReadCount < 3){ // for testing purpose
      byte arr[100];
      MyPrinter_println("OKAY serial " + String(SerialArray[0], HEX));
      switch(SerialArray[0] - 0x30){
        case 0: // Get my mesh uid || 0xAA, 0x55, 0x00, 0x03, 0xFF, 0x00
          makeUpSerialArr(arr, 0, 0x1A); // Init device event
          break;
        case 1: // Check if connected to mesh
          makeUpSerialArr(arr, 0, 0x10); // Create Instance Request || 0xAA, 0x55, 0x00, 0x04, 0xFF, 0x00
          // Will get a Create Instance Response || 0xAA, 0x55, 0x01, 0x05, [0x00/0x01] 0xFF, 0x00
          break;
        case 2: // Camera sending request to nurse terminal to get nurse terminal ID || 
          arr[0] = 0;//4E;
          arr[1] = 0;//54;
          // arr[2] = 0;//54;
          // arr[3] = 0;//54;
          // arr[4] = 0x00;
          // arr[5] = 0x00;
          makeUpMessage(0x4E54, 0xCEC4, arr, 2, 0); // Request an initial response from Terminal
          // Will get a Start Node Response from the terminal || 0xAA, 0x55, 0x04, 0x07 0x4E, 0x54, HibyteTID, LobyteTID 0xFF, 0x00
          break;
        case 3: //Send bed command
          arr[0] = 0x01; //command message type
          arr[1] = 0x01; //Reset command
          makeUpCommand(arr);
          break;
        case 4: //send patient Coordinate
          // arr[0] = 0x03; // patient coordinates message type
          //Sample bed coordinate ||  (200, 100), (200, 400), (600,, 100), (600, 400)
          arr[0] = 0;
          arr[1] = 1;
          arr[2] = highByte(127);
          arr[3] = lowByte(127);
          arr[4] = highByte(816);
          arr[5] = lowByte(816);
          makeUpMessage(0x4E54, 0xCEC4, arr, 4, 3);//bed coordinate is sent from cam to terminal
          break;
      }
    }
    if(goodSerial(SerialArray, ReadCount)){
      printArray("Read Bytes: ", SerialArray, ReadCount);
      processIncoming(SerialArray);
    }
    else
      MyPrinter_println("Bad serial");
  }
}
/**/
/*
16  FFFF
32  FFFF FFFF 
64  FFFF FFFF FFFF FFFF
128 FFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF

*/