#include <Arduino.h>

#include "painlessMesh.h"

#define MESH_SSID "Camertronix"
#define MESH_PASS "Fall Prention"
#define MESH_PORT 5555

Scheduler deviceScheduler;
painlessMesh mesh;
bool ConnectedToNet = false;
// #define _SERIAL_DEBUG

byte Preamble0 = 0xAA/*(int)'@'*/, Preamble1 = 0x55/*(int)'~'*/, HiByte = 0xFF/*(int)'#'*/, LoByte = 0x00/*(int)'$'*/;
/* this will hold the feedback from the mcu if the message packet has been received through serial
  The modem will only send a delivery feedback when the mcu confirms message reception over serial
  if this variable is true, then it means the modem is not waiting for any serial message received confirmation for the mcu
  So if true, the modem can attend to incoming messages, else will keep sending current received mesh message every 2 seconds over serial
  Once a confirmation is received it goes from false to true. If false, no incoming mesh message will be attended to*/ 
bool OpenToReceivedMesh = true;
   
void MyPrinter_println(String printstr){
    /*
    if(WiFi.status() == WL_CONNECTED){
    HTTPClient client;
    client.begin("http://192.168.100.147/fp?text=25: " + printstr + "\n");
    client.GET();
    client.end();
    }*/
    #ifdef _SERIAL_DEBUG
      Serial.println(printstr);
    #endif
}

void MyPrinter_print(String printstr){
    /*
    if(WiFi.status() == WL_CONNECTED){
    HTTPClient client;
    client.begin("http://192.168.100.147/fp?text=25: " + printstr);
    client.GET();
    client.end();
    }*/
    #ifdef _SERIAL_DEBUG
      Serial.print(printstr);
    #endif
}

bool goodSerial(byte Arr[200], byte len){
  if(Arr[0] == Preamble0 && Arr[1] == Preamble1 && Arr[len-2] == HiByte && Arr[len-1] == LoByte)
    return 1;
  return 0;
}

void printArray(String heading, byte Arr[200], byte len){
  MyPrinter_println(heading + " of length " + String(len));
  for(byte i = 0; i < len; i++)
    MyPrinter_print("0x" + String(Arr[i], HEX) + ", ");
  MyPrinter_println("");
}

void printStringBytes(String heading, String msg){
  MyPrinter_println(heading + "of length  " + String(msg.length()));
  for(byte i = 0; i < msg.length(); i++)
    MyPrinter_print("0x" + String((int)(msg.charAt(i)), HEX) + ", ");
  MyPrinter_println("\n");
}

void newConnectionCallback(uint32_t nodeId) {
  MyPrinter_println("New Connection, nodeId = " + String(nodeId));
  ConnectedToNet = true;
}

void changedConnectionCallback() { 
  MyPrinter_println("Changed connections\n"); 
}

void nodeTimeAdjustedCallback(int32_t offset) {
  // MyPrinter_println("Adjusted time" + String(mesh.getNodeTime()) + "  Offset = " + String(offset));
}

bool sendMessage(byte msg_arr[200], byte msg_len){
  String sending_msg = "";
  for(byte i = 0; i < msg_len; i += 1){
    // if(!msg_arr[i])
    if(i > 1 && i != msg_len-2 && msg_arr[i] < 0xFF)
      msg_arr[i] += 1;
    sending_msg += (char)(msg_arr[i]);
  }
  MyPrinter_println("Sending message: " + sending_msg);
  // Serial.write(sending_msg.c_str());
  printStringBytes("Sent message: ", sending_msg);
  if(mesh.sendBroadcast(sending_msg))
    return 1;
  return 0;
}

void receivedCallback(uint32_t from, String &msg) {
  byte msg_arr[200], msg_len = msg.length();
  MyPrinter_println("Message received from " + String(from) + " of length " + String(msg_len) + "\n" + msg);
  printStringBytes("Incoming message: ", msg);
  
  //Convert the string to a byte array
  for(byte i = 0; i < msg_len; i++){
    msg_arr[i] = (int)(msg.charAt(i));
    // if(msg_arr[i] == 0x7F)
    if(i > 1 && i != msg_len-2)
      msg_arr[i] -= 1;
  }
    printArray("Byte message: ", msg_arr, msg_len);
  if(goodSerial(msg_arr, msg_len)){
    printArray("Checked message: ", msg_arr, msg_len);
    Serial.write(msg_arr, msg_len);
    // Send a feedback if it's a message [don't send feedback on feedback]
      // if(msg_arr[3] == 0x07){
      //   msg_arr[2] = 10; //change the length to 10 (length of feedback arr)
      //   msg_arr[3] = 0x0F ; //message delivery command
      //   msg_arr[8] = HiByte; msg_arr[9] = LoByte;
      //   sendMessage(msg_arr, 10);
      // }
  }
  
}

void initCom(){
  mesh.setDebugMsgTypes(ERROR | STARTUP | DEBUG);  // set before init() so that you can see startup messages

  mesh.init(MESH_SSID, MESH_PASS, &deviceScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
}
/**/