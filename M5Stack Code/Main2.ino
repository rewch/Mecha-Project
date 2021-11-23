#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <M5Stack.h>
#include<Keypad.h>
#include <TridentTD_LineNotify.h>
#include <PubSubClient.h>
#include "MFRC522_I2C.h"
#include <Wire.h>
#include <analogWrite.h>
MFRC522 mfrc522(0x28); 
///////Color///////////
int bg = 0x03EF;
int lockcolor = 0xFDA0;
int Textcolor = BLACK ;
bool lockscr=false;
bool scr = false;
///// Wifi /////////////////
#define ssid  "TrueGigatexMesh_752" 
#define password  "0898820408" 

///// Line /////////////////
#define LINE_TOKEN "VYofSoLoG3aRvQg6NsCxWhKssLlf1F1owo5ADHp8DoF"
String txt = "Vault is Opened"; 
String txt2 = "Vault is Closed";
String txt3 = "Vault is Locked";
///// Netpie-MQTT /////////////////
#define mqtt_server "broker.netpie.io"
const int mqtt_port = 1883;
#define mqtt_Client "94a4a70f-e74c-43cc-a77f-2eab96172458"
#define mqtt_username "ZUA9piPysjgbjeHJnTaX2siUVA68TmMu"
#define mqtt_password "BS#nj6rH*Heo)xRfUQ#H(fLfRNgoIncp"
#define Subscribe_Topic "@msg/key/verified"
#define Publish_Topic0 "@msg/esp32/reset_state"
#define Publish_Topic2 "@msg/esp32/encoder_password"
#define Publish_Topic1 "@msg/esp32/keyboard_password"
#define Publish_Topic3 "@msg/esp32/line_request_password"
#define Publish_Topic4 "@msg/esp32/RFID_password"
#define Publish_Topic5 "@msg/esp32/line_verified"
unsigned long previousMillis = 0;
WiFiClient espClient;
PubSubClient client(espClient);
char msg[50];
String message_callback;
String retained_message;
bool pwd_ready = false;

//Lcd 320x240 ///////////////
int color[] = {
  0xfd79, 0xe8e4, 0xfbe4, // pink red orange
  0xff80, 0x2589, 0x51d,  // yellow green blue
  0x3a59, 0xa254, 0x7bef, // dark_blue violet gray
  0xffff // white  
};
//////////////////////////////
enum State {  Standby, Keyboards ,Encoder ,RFID,Two_Ways, Opened , Locked };
enum State myState;
int attempt = 3; /////////////////////////////////////// Test with 3 attempt

///// Encoder /////////////////
const int phaseA = 2 ;
const int phaseB = 13 ;
const int Button = 15 ;
#define GET_CODE() uint8_t (digitalRead(phaseA) << 4 | digitalRead(phaseB))
int32_t count_change = 0 ;
uint8_t code = 0;
uint8_t code_old = 0;
int count_value_change = 0 ;
String pwd = "";
enum StateB { d0, d1, d2, d3, d4 ,d5, d6 , Check2, Verified2};
enum StateB Encoder_State;
int previous_value = 1; 

///// Keyboard /////////////////
bool key_recieved = false ;
int keyoutput;
const byte ROWS = 3;
const byte COLS = 3;
byte rowPins[ROWS] = {17,16,1}; 
byte colPins[COLS] = {18,5,3};

char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'}
}; 
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
String kbpwd = "";
enum StateK { k0, k1, k2, k3, k4 ,k5, k6 ,Check1, Verified1};
enum StateK KB_State;

/////RFID/////
enum StateR { Input, Check3, Verified3 };
enum StateR RFID_State;
String Card_ID = "";
bool Check_Card = false;

///// Locked ////////////
unsigned long myTime;
unsigned long start_time;
float timer_time = 15000 ; ////////////////////////////////// timerLockedtime //////////////////
bool start_count = false ;
int remaining_time ;


///// Opened /////
int motorPin = 12;
enum StateO {Opening, Open, Close};
enum StateO Opened_State;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  M5.begin();
  Wire.begin();
  mfrc522.PCD_Init();  
  initialize_encoder_setup();
  initialize_connection();
  myState = Standby;
  Encoder_State = d0;
  KB_State = k0;
  RFID_State = Input;
  pinMode(motorPin, OUTPUT);
  Opened_State = Opening;
  M5.Lcd.setBrightness(120);
  M5.Lcd.fillScreen(bg);
  M5.Lcd.setCursor(60,30,4);
  scr=true;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {
  M5.update();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  myTime = millis();
  switch(myState)
  {
    case Standby :{
      M5.Lcd.setTextColor(Textcolor,bg);
      if (scr==true){
        screendecor(1,lockcolor,bg);
        scr=false;
      }
      if (M5.BtnC.wasReleased()) {
        myState = Keyboards; 
        M5.Lcd.clear(bg);
        scr=true;
      }
    }break ;
    case Keyboards :{
      M5.Lcd.setTextColor(Textcolor,bg);
      M5.Lcd.setCursor(60,45,4);
      M5.Lcd.setTextSize(1);
      M5.Lcd.println("Phase 1 : Keyboard ");
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(WHITE,BLACK);
      if (scr==true){
         lockdecor(4,lockcolor,bg);
        scr=false;
      }
      char key = keypad.getKey() ;
      if (key){
        key_recieved = true;
        keyoutput = int(key)-48;
      }
      kb_input_pw();
      if (KB_State == Verified1 ){
          myState = Encoder;
          KB_State = k0 ;
          scr=true;
      }
    }break;    
    case Encoder :{
      M5.Lcd.setTextColor(Textcolor,bg);
      M5.Lcd.setCursor(48,45,4);
      M5.Lcd.setTextSize(1);
      M5.Lcd.println("Phase 2 : Encoder");
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(WHITE,BLACK);
      uint8_t value = digitalRead(Button);
      encoderEvent();
      if (scr==true){
         lockdecor(3,lockcolor,bg);
        scr=false;
      }
      input_encoder_password(value);
      if (Encoder_State == Verified2 ){
        myState = RFID;
        Encoder_State = d0 ;
        scr=true;
      }
    }break;

    case RFID :{
      M5.Lcd.setTextColor(Textcolor,bg);
      M5.Lcd.setCursor(48,30,4);
      M5.Lcd.setTextSize(1);
      M5.Lcd.println("Phase 3 : RFID");
      M5.Lcd.setCursor(30,58,4);
      M5.Lcd.setTextSize(1);
      M5.Lcd.println("Use your Card");
      M5.Lcd.setTextColor(WHITE,BLACK);
      M5.Lcd.setTextSize(2);
      input_RFID_password();
      if (scr==true){
         lockdecor(2,lockcolor,bg);
        scr=false;
      }
      if (RFID_State == Verified3) {
        myState = Two_Ways;
        RFID_State = Input;
        scr=true;
        M5.Lcd.clear(bg);
      }
    }break;

    case Two_Ways : {
      M5.Lcd.setTextColor(Textcolor,bg);
      M5.Lcd.setCursor(48,30,4);
      M5.Lcd.setTextSize(1);
      M5.Lcd.println("Phase 4 : Line");
      M5.Lcd.setCursor(15,58,4);
      M5.Lcd.setTextSize(1);
      M5.Lcd.println("Press C to get password");
      M5.Lcd.setTextColor(WHITE,BLACK);
      M5.Lcd.setTextSize(2);
      if (scr==true){
         lockdecor(1,lockcolor,bg);
        scr=false;
      }
      char key = keypad.getKey() ;
      if (key){
        key_recieved = true;
        keyoutput = int(key)-48;
      }
      
      line_authentification();
      if (KB_State == Verified1 ){
          myState = Opened;
          KB_State = k0 ;
          M5.Lcd.clear(bg);
          scr=true;
      }
     }break;

    case Opened:{
        if (scr==true){
         screendecor(5,lockcolor,bg);
        scr=false;
      }
        Opened_System();
        if (Opened_State == Close) {
          myState = Standby;
          M5.Lcd.clear(bg);
          String payload = "reset" ;
          char msg[50];
          payload.toCharArray(msg, (payload.length() +1 ));
          client.publish(Publish_Topic0,msg);
          scr=true;
        }
        
      }break;

    case Locked :{
      if (scr==true){
        screendecor(3,lockcolor,bg);
        scr = false;
         LINE.notify(txt3);  
      }
      timer(timer_time, true ) ;
      String payload = "correct" ;
      
    }break;
  }
  if (attempt == 0) {
    M5.Lcd.clear(bg);
    myState = Locked;
    attempt = 3;
    scr = true;
  }

  if (M5.BtnB.wasReleased() && myState != Locked) {
        myState = Standby; 
        Encoder_State = d0;
        KB_State = k0;
        RFID_State = Input; ////////////////////
        pwd = "" ;
        kbpwd = "" ;
        Card_ID = ""; ////////////////////////
        M5.Lcd.clear(bg);
        String payload = "reset" ;
        char msg[50];
        payload.toCharArray(msg, (payload.length() +1 ));
        client.publish(Publish_Topic0,msg);
        scr=true;
      }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void initialize_encoder_setup() {
  pinMode(phaseA,INPUT_PULLUP);
  pinMode(phaseB,INPUT_PULLUP);
  pinMode(Button,INPUT_PULLUP);
  dacWrite(25,0);
  code = GET_CODE();
  code_old = code ;
  
}

void encoderEvent() {
  code = GET_CODE();
  if (code != code_old) {
    if (code == 0x00) {
      if(code_old == 0x10) {
        count_change--;
      }
      else {
        count_change++ ;
      }
    }
    code_old = code ;
  }
  if (count_change%2 == 0) {
    count_value_change = int(count_change/2) ;
  }
  if (count_value_change>=10) {
    count_value_change = 0;
    count_change = 0 ;
  }
  else if (count_value_change <=-1) {
    count_value_change = 9 ;
    count_change = 18 ;
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void initialize_password(int x_start, int y,int x_interval, int digit) {
  M5.Lcd.setTextSize(2);
  int x = x_start;
  for ( int i= 0; i< digit ;i++) {
    M5.Lcd.setCursor(x,y);
    M5.Lcd.println("_");
    x = x + x_interval;
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void input_encoder_password(int value) {
  switch (Encoder_State)
  {
    case d0 :
      initialize_password(50,100,40,6);
      delay(100);
      Encoder_State = d1;
      break ;
    case d1 :
      M5.Lcd.setCursor(50,90);
      M5.Lcd.printf("%01d", count_value_change);
      if (previous_value == 1 && value == 0) {
        Encoder_State = d2; 
        pwd+=count_value_change;
      }
      break ;
    case d2 :
      M5.Lcd.setCursor(90,90);
      M5.Lcd.printf("%01d", count_value_change);
      if (previous_value == 1 && value == 0) {
        Encoder_State = d3; 
        pwd+=count_value_change;
      }
      break ;
    case d3 :
      M5.Lcd.setCursor(130,90);
      M5.Lcd.printf("%01d", count_value_change);
      if (previous_value == 1 && value == 0) {
        Encoder_State = d4; 
        pwd+=count_value_change;
      }

      break ;
    case d4 :
      M5.Lcd.setCursor(170,90);
      M5.Lcd.printf("%01d", count_value_change);
      if (previous_value == 1 && value == 0) {
        Encoder_State = d5; 
        pwd+=count_value_change;
      }
      break ;
    case d5 :
      M5.Lcd.setCursor(210,90);
      M5.Lcd.printf("%01d", count_value_change);
      if (previous_value == 1 && value == 0) {
        Encoder_State = d6; 
        pwd+=count_value_change;
      }
      break ;
    case d6 :
      M5.Lcd.setCursor(250,90);
      M5.Lcd.printf("%01d", count_value_change);
      if (previous_value == 1 && value == 0) {
        Encoder_State = Check2; 
        pwd+=count_value_change;
        pwd_ready = true ;
        M5.Lcd.clear(bg);
      }
      break ;
    case Check2 :
      M5.Lcd.setCursor(50,90);
      M5.Lcd.setTextSize(1) ; 
      M5.Lcd.setTextColor(Textcolor,bg);
      M5.Lcd.println("Verifying Password");
      if (pwd_ready == true) {
        pwd_ready = false ;
        String payload = pwd ;
        char msg[50];
        payload.toCharArray(msg, (payload.length() +1 ));
        client.publish(Publish_Topic2,msg);
      }
      if (message_callback == "correct") { 
        message_callback= "";
        M5.Lcd.clear(bg);
        Encoder_State = Verified2;
        attempt = 3 ;
        pwd = "";
      }
      else if (message_callback == "incorrect") {
        message_callback= "";
        attempt-- ;
        if (attempt == 0){
          Encoder_State = d0;
          KB_State = k0;
          break;
        }
        M5.Lcd.clear(bg);
        M5.Lcd.setCursor(50,70);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(Textcolor,bg);
        M5.Lcd.println("Incorect Password");
        M5.Lcd.println("");
        M5.Lcd.printf("       Remaining Attempt: %01d", attempt);
        delay(1500);
        M5.Lcd.clear(bg);
        Encoder_State = d0;
        pwd = ""  ;
        scr = true;
     }
     message_callback= "";
     break ;
  } 
  if (M5.BtnA.wasReleased()) {
        Encoder_State = d0;
        pwd = "";
        M5.Lcd.clear(bg);
      }
  previous_value = value ;
  delay(5);

}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void kb_input_pw() {
  switch (KB_State)
  {
    case k0 : 
      initialize_password(50,100,40,6);
      delay(1000);
      KB_State = k1;
      break ;
    case k1 :
      M5.Lcd.setCursor(50,90);
      if (key_recieved == true){
          M5.Lcd.printf("%01d", keyoutput);
          kbpwd+=keyoutput;
          KB_State = k2 ;
          key_recieved = false ;
      }
      break;
    case k2 :
      M5.Lcd.setCursor(90,90);
      if (key_recieved == true){
          M5.Lcd.printf("%01d", keyoutput);
          kbpwd+=keyoutput;
          KB_State = k3 ;
          key_recieved = false ;
      }
      break;
    case k3 :
      M5.Lcd.setCursor(130,90);
      if (key_recieved == true){
          M5.Lcd.printf("%01d", keyoutput);
          kbpwd+=keyoutput;
          KB_State = k4 ;
          key_recieved = false ;
      }
      break;
    case k4 :
      M5.Lcd.setCursor(170,90);
      if (key_recieved == true){
          M5.Lcd.printf("%01d", keyoutput);
          kbpwd+=keyoutput;
          KB_State = k5 ;
          key_recieved = false ;
      }
      break;
    case k5 :
      M5.Lcd.setCursor(210,90);
      if (key_recieved == true){
          M5.Lcd.printf("%01d", keyoutput);
          kbpwd+=keyoutput;
          KB_State = k6 ;
          key_recieved = false ;
      }
      break;
    case k6 :
      M5.Lcd.setCursor(250,90);
      if (key_recieved == true){
          M5.Lcd.printf("%01d", keyoutput);
          kbpwd+=keyoutput;
          KB_State = Check1;
          key_recieved = false ;
          pwd_ready = true;
          M5.Lcd.clear(bg);
      }
      break;  
    case Check1 :
      M5.Lcd.setCursor(50,90);
      M5.Lcd.setTextSize(1) ; 
      M5.Lcd.setTextColor(Textcolor,bg);
      M5.Lcd.println("Verifying Password");
      if (myState == Keyboards) {
        if (pwd_ready == true) {
          pwd_ready = false ;
          String payload = kbpwd ;
          char msg[50];
          payload.toCharArray(msg, (payload.length() +1 ));
          client.publish(Publish_Topic1,msg);
        }
      }
      if (message_callback == "correct" ) { 
        message_callback= "";
        M5.Lcd.clear(bg);
        KB_State = Verified1;
        attempt = 3 ;
        kbpwd = ""  ;
      }
      else if (message_callback == "incorrect" ) {
        attempt-- ;
        message_callback= "";
        if (attempt == 0){
          KB_State = k0;
          break;
        }
        M5.Lcd.clear(bg);
        M5.Lcd.setCursor(50,70);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(Textcolor,bg);
        M5.Lcd.println("Incorect Password");
        M5.Lcd.println("");
        M5.Lcd.printf("       Remaining Attempt: %01d", attempt);
        delay(1500);
        M5.Lcd.clear(bg);
        KB_State = k0;
        kbpwd = ""  ;
        scr= true;
       }
       break ;
  }
  retained_message = message_callback; 
  if (M5.BtnA.wasReleased()) {
        KB_State = k0;
        kbpwd = ""  ;
        M5.Lcd.clear(bg);
      }
  delay(15);
          
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void initialize_RFID_setup() {
  if(!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
  {
    Check_Card = false;
    delay(50);
    return;
  }
  else {
    Check_Card = true;
  }
  dacWrite(25,0);
}

void input_RFID_password(){
  switch (RFID_State)
  {
    case Input : {      
      Card_ID = ""  ;
      initialize_RFID_setup();  
      for(byte i=0; i<mfrc522.uid.size; i++) {
        Card_ID += String(mfrc522.uid.uidByte[i],HEX);
      }
      if (Check_Card == true){
        Check_Card = false;
        RFID_State = Check3;
        pwd_ready = true ;
      }
    }
      break;
    case Check3 : {
      M5.Lcd.setCursor(50,90);
      M5.Lcd.setTextSize(1) ; 
      M5.Lcd.setTextColor(Textcolor,bg);
      M5.Lcd.println("Verifying Password");
      if (pwd_ready == true) {
        pwd_ready = false ;
        String payload = Card_ID ;
        char msg[50];
        payload.toCharArray(msg, (payload.length() +1 ));
        client.publish(Publish_Topic4,msg);
      }
      if (message_callback == "correct") { 
        message_callback= "";
        M5.Lcd.clear(bg);
        RFID_State = Verified3;
        attempt = 3 ;
        Card_ID = ""; /////////////////////////
      }
      else if (message_callback == "incorrect") {
        message_callback= "";
         attempt-- ;
        if (attempt <= 0){
          break;
        }
        M5.Lcd.clear(bg);
        M5.Lcd.setCursor(50,70);
        M5.Lcd.setTextSize(1.5);
        M5.Lcd.setTextColor(Textcolor,bg);
        M5.Lcd.println("Incorect CARD");
        M5.Lcd.println("");
        M5.Lcd.printf("       Remaining Attempt: %01d", attempt);
        delay(1500);
        M5.Lcd.clear(bg);
        Card_ID = ""  ;
        RFID_State = Input;
        scr = true;
        
      }
    }
  }
  if (M5.BtnA.wasReleased()) {
        RFID_State = Input;
        M5.Lcd.clear(bg);
      }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void timer(float timer_time , bool lcd_show ) {
     if (start_count== false){
        start_count = true ;
        start_time  = myTime ;
     }
     remaining_time =  int((timer_time - (myTime-start_time)))/1000 ;
     if (lcd_show) {
        M5.Lcd.setCursor(40,20);
        M5.Lcd.setTextColor(BLACK,RED);
        if ((timer_time - (myTime-start_time))/1000 == 9.0){
          M5.Lcd.clear(bg);
          scr=true;
        }
        M5.Lcd.printf("Try again in %2d second", remaining_time);
     }
     if (remaining_time==0) {
        M5.Lcd.clear(bg);
        myState = Standby ;
        start_count = false ;
        String payload = "reset" ;
        char msg[50];
        payload.toCharArray(msg, (payload.length() +1 ));
        client.publish(Publish_Topic0,msg);
        scr = true;
     }
     
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void initialize_connection() {
  WiFi.begin(ssid,password);
  while (WiFi.status() != WL_CONNECTED) {
//    M5.Lcd.setCursor(0,180);
//    M5.Lcd.println(WiFi.status());
//    M5.Lcd.println("Connecting to Wifi ....");
    delay(500);
  }
//  M5.Lcd.setCursor(0,150);
//  M5.Lcd.clear(BLACK);
//  M5.Lcd.println("Connected to Wifi ");
//  M5.Lcd.println(WiFi.localIP());
//  M5.Lcd.clear(BLACK);
  LINE.setToken(LINE_TOKEN);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect(mqtt_Client, mqtt_username, mqtt_password)) {
      client.subscribe(Subscribe_Topic);
    } else {
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  
  for (int i = 0; i < length; i++) {
    message_callback = message_callback + (char)payload[i];
  }
//  M5.Lcd.setCursor(100,150);
//  M5.Lcd.println(message_callback);
//  if (String(topic) == Subscribe_Topic) {
//
//  }
}

void Opened_System() {
  switch (Opened_State)
  {
    case Opening : {
      LINE.notify(txt);
      analogWrite(motorPin,200);
      delay(3000);
      analogWrite(motorPin,0);
      Opened_State = Open;
    }
    break;
    case Open : {
      if (M5.BtnC.wasReleased()) {
        screendecor(2,3,4);
        analogWrite(motorPin,200);
        delay(3000);
        analogWrite(motorPin,0);
        Opened_State = Close;
        LINE.notify(txt2);
      }
    }
    break;
    
  }
  
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void line_authentification() {
   if (M5.BtnC.wasReleased()) {
        String payload = "Request for Password" ;
        payload.toCharArray(msg, (payload.length() + 1));
        client.publish(Publish_Topic3,msg);
   }
   kb_input_pw();
   if (retained_message == kbpwd  && KB_State == Check1 ) {
        String payload = "correct" ;
        char msg[50];
        payload.toCharArray(msg, (payload.length() +1 ));
        client.publish(Publish_Topic5,msg);
        message_callback= "";
        M5.Lcd.clear(bg);
        KB_State = Verified1;
        attempt = 3 ;
        kbpwd = ""  ;
   }
   else if (retained_message != kbpwd  && KB_State == Check1 ) {
        attempt-- ;
        if (attempt == 0){
          message_callback= "";
          KB_State = k0;
        }
        else {
        M5.Lcd.clear(bg);
        M5.Lcd.setCursor(50,70);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(Textcolor,bg);
        M5.Lcd.println("Incorect Password");
        M5.Lcd.println("");
        M5.Lcd.printf("       Remaining Attempt: %01d", attempt);
        delay(1500);
        M5.Lcd.clear(bg);
        KB_State = k0;
        kbpwd = ""  ;
        scr = true ;
       }
   }
}
void lockdecor(int d,int color,int bg){
  if (d == 4)/////keyboard////
  {
  M5.Lcd.fillCircle(75,200, 11, color);
  M5.Lcd.fillCircle(75,200, 7, bg);
  M5.Lcd.fillRect(60, 200, 30,20,color );
  M5.Lcd.fillCircle(135,200, 11, color);
  M5.Lcd.fillCircle(135,200, 7, bg);
  M5.Lcd.fillRect(120, 200, 30,20,color );
  M5.Lcd.fillCircle(195,200, 11, color);
  M5.Lcd.fillCircle(195,200, 7, bg);
  M5.Lcd.fillRect(180, 200, 30,20,color );
  M5.Lcd.fillCircle(255,200, 11, color);
  M5.Lcd.fillCircle(255,200, 7, bg);
  M5.Lcd.fillRect(240, 200, 30,20,color);
  M5.Lcd.fillCircle(30,55, 10, color);
  M5.Lcd.progressBar(0,0,320,20, 0);
  }
  else if (d == 3)////encode////
  {
  M5.Lcd.fillCircleHelper(95, 200, 11, 150,0,color);
  M5.Lcd.fillCircleHelper(95, 200, 7, 150,0, bg);  
  M5.Lcd.fillRect(60, 200, 30, 20,color );
  M5.Lcd.fillCircle(135,200, 11, color);
  M5.Lcd.fillCircle(135,200, 7, bg);
  M5.Lcd.fillRect(120, 200, 30,20,color );
  M5.Lcd.fillCircle(195,200, 11, color);
  M5.Lcd.fillCircle(195,200, 7, bg);
  M5.Lcd.fillRect(180, 200, 30,20,color );
  M5.Lcd.fillCircle(255,200, 11, color);
  M5.Lcd.fillCircle(255,200, 7, bg);
  M5.Lcd.fillRect(240, 200, 30,20,color);
  M5.Lcd.fillCircle(30,55, 10, color);
  M5.Lcd.progressBar(0,0,320,20, 25);
  }
  else if (d == 2)///RFID////
  {
  M5.Lcd.fillCircleHelper(95, 200, 11, 150,0,color);
  M5.Lcd.fillCircleHelper(95, 200, 7, 150,0, bg);  
  M5.Lcd.fillRect(60, 200, 30, 20,color );
  M5.Lcd.fillCircleHelper(155, 200, 11, 150,0,color);
  M5.Lcd.fillCircleHelper(155, 200, 7, 150,0, bg);  
  M5.Lcd.fillRect(120, 200, 30, 20,color );
  M5.Lcd.fillCircle(195,200, 11, color);
  M5.Lcd.fillCircle(195,200, 7, bg);
  M5.Lcd.fillRect(180, 200, 30,20,color );
  M5.Lcd.fillCircle(255,200, 11, color);
  M5.Lcd.fillCircle(255,200, 7, bg);
  M5.Lcd.fillRect(240, 200, 30,20,color);
  M5.Lcd.fillCircle(30,40, 10, color);
  M5.Lcd.progressBar(0,0,320,20, 50);
}
 else if (d == 1)///LINE////
 {
  M5.Lcd.fillCircleHelper(95, 200, 11, 150,0,color);
  M5.Lcd.fillCircleHelper(95, 200, 7, 150,0, bg);  
  M5.Lcd.fillRect(60, 200, 30, 20,color );
  M5.Lcd.fillCircleHelper(155, 200, 11, 150,0,color);
  M5.Lcd.fillCircleHelper(155, 200, 7, 150,0, bg);  
  M5.Lcd.fillRect(120, 200, 30, 20,color );
  M5.Lcd.fillCircleHelper(216, 200, 11, 150,0,color);
  M5.Lcd.fillCircleHelper(216, 200, 7, 150,0, bg);  
  M5.Lcd.fillRect(180, 200, 30, 20,color );
  M5.Lcd.fillCircle(255,200, 11, color);
  M5.Lcd.fillCircle(255,200, 7, bg);
  M5.Lcd.fillRect(240, 200, 30,20,color);
  M5.Lcd.fillCircle(30,40, 10, color);
  M5.Lcd.progressBar(0,0,320,20, 75);
}
}
void screendecor(int d,int color,int bg){
  if (d==1)///interface// 
  {
    M5.Lcd.setCursor(100,20,4);
    M5.Lcd.setTextSize(1);
    M5.Lcd.println("Digital  Vault");
    M5.Lcd.fillCircleHelper(160, 100, 40, 150,0,color);
    M5.Lcd.fillCircleHelper(160, 100, 30, 150,0, bg);  
    M5.Lcd.fillRect(120, 100, 80, 70,color );
    M5.Lcd.fillCircle(160, 128, 10,bg);
    M5.Lcd.fillRect(155, 130, 10, 20,bg );
    M5.Lcd.setCursor(76,200,4);
    M5.Lcd.setTextSize(1);
    M5.Lcd.println("Press C to login ");
    M5.Lcd.fillRect(260, 10, 50, 10,RED);
    M5.Lcd.fillRect(260, 30, 50, 10, ORANGE);
    M5.Lcd.fillRect(260, 50, 50, 10, YELLOW);
    M5.Lcd.fillRect(260, 70, 50, 10, GREENYELLOW);
    M5.Lcd.fillRect(260, 90, 50, 10, GREEN);
    M5.Lcd.fillRect(260, 110, 50, 10, BLUE);
    M5.Lcd.fillRect(260, 130, 50, 10, NAVY);
    M5.Lcd.fillRect(260, 150, 50, 10, PURPLE);
    M5.Lcd.fillRect(260, 170, 50, 10, PINK);
    M5.Lcd.fillRect(260, 190, 50, 10, 0x7BEF);
    M5.Lcd.fillRect(260, 210, 50, 10,BLACK);
    M5.Lcd.fillRect(10, 10, 50, 10, RED);
    M5.Lcd.fillRect(10, 30, 50, 10, ORANGE);
    M5.Lcd.fillRect(10, 50, 50, 10, YELLOW);
    M5.Lcd.fillRect(10, 70, 50, 10, GREENYELLOW);
    M5.Lcd.fillRect(10, 90, 50, 10, GREEN);
    M5.Lcd.fillRect(10, 110, 50, 10, BLUE);
    M5.Lcd.fillRect(10, 130, 50, 10, NAVY);
    M5.Lcd.fillRect(10, 150, 50, 10, PURPLE);
    M5.Lcd.fillRect(10, 170, 50, 10, PINK);
    M5.Lcd.fillRect(10, 190, 50, 10, 0x7BEF);
    M5.Lcd.fillRect(10, 210, 50, 10, BLACK);
  }
  else if (d==2)//Closing//
  {
   
    M5.Lcd.fillScreen(GREEN);
    M5.Lcd.setCursor(80,20,4);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(BLACK,GREEN);
    M5.Lcd.println("Vault is Closing");
//    M5.Lcd.setCursor(100,50,4);
//    M5.Lcd.setTextSize(1);
//    M5.Lcd.println("Thank you !");
    M5.Lcd.fillCircle(160, 140, 50,YELLOW);
    M5.Lcd.fillCircle(140, 125, 10,BLACK);
    M5.Lcd.fillCircle(180, 125, 10,BLACK);
    M5.Lcd.setRotation(3);
    M5.Lcd.fillCircleHelper(160, 90, 20, 150,0,RED);
    M5.Lcd.fillCircleHelper(160, 90, 14, 150,0,YELLOW);
    M5.Lcd.setRotation(9);
  }
  else if (d==3)//Lock countdown//
  {
    M5.Lcd.fillScreen(RED);
    M5.Lcd.setCursor(70,60);
    M5.Lcd.setTextSize(1.5);
    M5.Lcd.setTextColor(BLACK,RED);
    M5.Lcd.println("No Attempt Left");
    M5.Lcd.fillCircle(160, 140, 50,YELLOW);
    M5.Lcd.fillCircle(140, 125, 10,BLACK);
    M5.Lcd.fillCircle(180, 125, 10,BLACK);
    M5.Lcd.fillCircleHelper(160,165,20,150,0,RED);
    M5.Lcd.fillCircleHelper(160,165,14,150,0,YELLOW);     
  }
  else if (d==5)//Unlock//
  {
    M5.Lcd.setCursor(75,40,4);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(GREEN,bg);
    M5.Lcd.println("Vault Unlocked!");
    M5.Lcd.fillCircleHelper(160, 120, 40, 150,0,color);
    M5.Lcd.fillCircleHelper(160, 120, 30, 150,0, bg); 
    M5.Lcd.fillRect(120, 120, 10, 20,color ); 
    M5.Lcd.fillRect(120, 140, 80, 70,color );
    M5.Lcd.fillCircle(160, 168, 10,bg);
    M5.Lcd.fillRect(155, 170, 10, 20,bg );
    M5.Lcd.setTextColor(BLACK,YELLOW);
    M5.Lcd.fillCircle(280, 70, 20,YELLOW);
    M5.Lcd.drawChar('$', 277, 63, 2);
    M5.Lcd.fillCircle(280, 130, 20,YELLOW);
    M5.Lcd.drawChar('$', 277, 123, 2);
    M5.Lcd.fillCircle(280, 190, 20,YELLOW);
    M5.Lcd.drawChar('$', 277, 183, 2);
    M5.Lcd.fillCircle(30, 70, 20,YELLOW);
    M5.Lcd.drawChar('$', 27, 63, 2);
    M5.Lcd.fillCircle(30, 130, 20,YELLOW);
    M5.Lcd.drawChar('$', 27, 123, 2);
    M5.Lcd.fillCircle(30, 190, 20,YELLOW);
    M5.Lcd.drawChar('$', 27, 183, 2);
    M5.Lcd.progressBar(0,0,320,20, 100);
  }
}
