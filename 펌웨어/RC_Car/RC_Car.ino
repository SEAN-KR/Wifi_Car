#include <M5Atom.h>
#include "AtomMotion.h"

AtomMotion Atom;

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <ESPConnect.h>

#include "MapFloat.h"
#include "Update.h"
#include "web.h"

extern int restartNow;
extern int updating;
  
String xval, yval;
float xvalue, yvalue;
int conv_x, conv_y;

WebSocketsServer webSocket = WebSocketsServer(81);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void remote_Action(String msg);
String getValue(String data, char separator, int index);

void setup(){
  M5.begin(true, false, true);
  delay(50);
  M5.dis.drawpix(0, 0xf00000);
  Atom.Init();     //sda  25     scl  21 
  
  Serial.begin(115200);
  Serial.println();
  /*
    AutoConnect AP
    Configure SSID and password for Captive Portal
  */
  ESPConnect.autoConnect("eddy0215@kakao.com");

  /* 
    Begin connecting to previous WiFi
    or start autoConnect AP if unable to connect
  */
  if(!ESPConnect.begin(&server)){
    Serial.println("Connected to WiFi");
    Serial.println("IPAddress: "+WiFi.localIP().toString());
  }else{
    Serial.println("Failed to connect to WiFi");
    ESPConnect.erase();
  }
  init_webpage();
  server.begin();
}

uint8_t FSM = 1;
bool isLoopWS = false;
bool isrecv = false;
String ws_msg;

void loop(){ 
  M5.update();
  if(M5.Btn.wasPressed()){
    switch (FSM)
        {
        case 0:
            M5.dis.drawpix(0, 0xf00000);
            server.begin();
            Serial.println("Start OTA");
            break;
        case 1:
            M5.dis.drawpix(0, 0x00f000);
            server.end();
            Serial.println("Stop OTA");
            webSocket.begin();
            webSocket.onEvent(webSocketEvent);
            isLoopWS = true;  
            Serial.println("Start WebSocket server");
            break;
        case 2:
            M5.dis.drawpix(0, 0x0000f0);
            webSocket.close();
            isLoopWS = false;
            break;
        case 3:
            M5.dis.drawpix(0, 0x707070);
            break;
        default:
            break;
        }
        FSM++;
        if (FSM >= 4)
        {
            FSM = 0;
        }
  }
  if (restartNow){
    Serial.println("Restart");
    ESP.restart();
  }
  if(updating){
    //timerAlarmDisable(timer);
  }
  if(isLoopWS)
    webSocket.loop();
  if(isrecv)  
    remote_Action(ws_msg);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
              IPAddress ip = webSocket.remoteIP(num);
              Serial.printf("[%u] Connection from ", num);
              Serial.println(ip.toString());
            }
            break;
        case WStype_TEXT:
            Serial.printf("[%u] Text: %s\n", num, payload);
            ws_msg = (char*)payload;
            webSocket.sendTXT(num, ws_msg);
            isrecv = true;
            break;
        case WStype_BIN:
        case WStype_ERROR:      
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            break;
    }
}

void remote_Action(String msg){
  Serial.printf("[WS from Text: %s\n", msg);
// JOYSTIC x:-1.0 y:-0.20755638  y=motor x=Steering
  xval = getValue(msg, ':', 1);
  yval = getValue(msg, ':', 2);
  Serial.println("xval: " + xval);
  Serial.println("yval: " + yval);
  
  xvalue = xval.toFloat();
  yvalue = yval.toFloat();
  Serial.println(xvalue);
  Serial.println(yvalue);

  conv_x = (int)mapFloat(xvalue, -1.0, 1.0, 110.0, 70.0);
  conv_y = (int)mapFloat(yvalue, -1.0, 1.0, -80.0, 80.0);
  Serial.println(conv_x);
  Serial.println(conv_y);

  isrecv = false;

  Atom.SetMotorSpeed(1, conv_y);
  Atom.SetServoAngle(2, conv_x); 
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
