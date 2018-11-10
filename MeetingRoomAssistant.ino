#include <TimeAlarms.h>

#include <SPI.h>
#include <Ethernet.h>
#include <RFID.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>



#define ID "13121236217170"

/*Attendants tracking related variables*/
#define IR_ANALOG_1 A0
#define IR_ANALOG_2 A1
#define IR_THRESHOLD 200
//Defines used to  track the direction of a person

#define NO_IR_SENSOR_TRIGGERED 0
#define IR1_TRIGGERED_FIRST 1
#define IR2_TRIGGERED_FIRST 2


/*Defines used for Temperature sensor*/
//Represents the pin on which the temperature signal is read.
#define TEMPERATURE_ADC A2
//Represents a coeficient that is used to deduce the temperature from ADC.
#define TEMPERATURE_COEF_PER_ADC 0.48828125

/*Varianble used to tracker the number of attendants at a meeting*/
int numberOfPersons=0;
boolean firstSensorTriggered=false;
boolean secondSensorTriggered=false;
/*Used to track which sensor triggered  first in order to determine direction */
int sensorTriggerDirection;
/*End of Attedants tracking related variables*/


#define SDA_DIO 9
#define RESET_DIO 8


/* Create an instance of the RFID library */
RFID RC522(SDA_DIO, RESET_DIO); 

//Initialization of lcd keypad module
// The shield uses the I2C SCL and SDA pins. On classic Arduinos
// this is Analog 4 and 5 so you can't use those for analogRead() anymore
// However, you can connect other I2C sensors to the I2C bus and share
// the I2C bus.
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
#define WHITE 0x7

#define MAIN_VIEW 0
#define MENU_ITEM_EXPAND 10
#define MENU_ITEM_EXPAND_SETTING 11
#define MENU_ITEM_EXIT 20

int currentView=0;
int extendMeetingIndicator=1;
int currentExtendCount=1;
#define MAX_EXTEND_COUNT 4
#define MIN_EXTEND_COUNT 1


//Variable used to store the state(enables/disabled) of the below mentioned task.
boolean T1_Disabled=false;
AlarmId dataSending_FreeMeetingRoom;
//Variable used to store the state(enables/disabled) of the below mentioned task.
boolean T2_Disabled=true;
AlarmId dataSending_OccupiedMeetingRoom;

//Variable used to store the state(enables/disabled) of the below mentioned task.
boolean autoCancelTask_Status;
AlarmId autoCancelTask;

/*Variable used to store to current view displayed on LCD*/
int lcdView=0;
//Ethernet shield initialization
byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x0E, 0x03, 0xE2};
IPAddress ip(192,168,0,251);
byte server[] = {192, 168,   0,  5};  
IPAddress myDns(192,168,0,1);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);
EthernetClient client;


void setup(){
  //Serial monitor initialization
  Serial.begin(9600);
  
  //Ethernet shield initialization. 
  Ethernet.begin(mac, ip, dns, gateway, subnet); 
   // Check connection to web service
  bool result=testConnection();
  if(result){
   Serial.println("Connection with Web Service is up");
   }else{
   Serial.println("Connection with Web Service is down");
  }
  
  RC522.init();
  
  lcd.begin(16, 2);               // start the library
  lcd.setCursor(0,0);             // set the LCD cursor position  
  lcd.setBacklight(WHITE);

  /*Define the task for regular task for free meeting room related information to server*/
  dataSending_FreeMeetingRoom= Alarm.timerRepeat(5,sendFreeMeetingRoomDataToServer);
  T1_Disabled=true;
  Alarm.disable(dataSending_FreeMeetingRoom);
  /*Define the task for regular task for occupied meeting room related information to server*/
  dataSending_OccupiedMeetingRoom=Alarm.timerRepeat(5,sendOccupiedMeetingRoomDataToServer);
  T2_Disabled=true;
  Alarm.disable(dataSending_OccupiedMeetingRoom);

  autoCancelTask=Alarm.timerRepeat(10,autoCancelMeetingTask);
  autoCancelTask_Status=true;
  Alarm.disable(autoCancelTask);
  
  delay(2000);

}


String AuthCardID;
boolean RoomAvailability = true;

int bookingTimeStamp;
int remainingTime;

int temperature=0;
int numberOfParticipants=0;
//Defines the duration of a meeting in minutes.
int BOCKING_PERIOD = 5;

void loop()
{   
Alarm.delay(500);

temperature= readTemperature();
//Serial.print("Temperature: ");
//Serial.println(temperature);
//Serial.println("Number of participants:");
//Serial.println(numberOfPersons);

RoomAvailability=false;
occupiedMeetingRoomBehavior();

 if( RoomAvailability == true){
    readCardIfAvailable();
  }

  if( RoomAvailability == true){
    freeMeetingRoomBehavior(); 
  }else{
    occupiedMeetingRoomBehavior();
  }
  
  if(remainingTime <= 0)
  {
    RoomAvailability = true;
  }   
}

void freeMeetingRoomBehavior(){
  if(T1_Disabled){
   Alarm.disable(dataSending_OccupiedMeetingRoom);
   T2_Disabled=true;
   Alarm.enable(dataSending_FreeMeetingRoom);
   T1_Disabled=false;
   }  
  numberOfPersons=0; 
}
void occupiedMeetingRoomBehavior(){
  if(T2_Disabled){
  Alarm.disable(dataSending_FreeMeetingRoom);
  T1_Disabled=true;
  Alarm.enable(dataSending_OccupiedMeetingRoom); 
  T2_Disabled=false; 
 }
  calculateRemainingTime();
  trackAttendants();
  handleMenuNagivation();
  renderView();
  if(numberOfPersons>0){
     printRemainingBookingTime();
     if(autoCancelTask_Status==true){
       Alarm.disable(autoCancelTask);
       autoCancelTask_Status=false;
       Serial.println("Auto-Cancel task has been disabled!"); 
      }
  }else{
    if(autoCancelTask_Status==false){
      Alarm.enable(autoCancelTask);
      autoCancelTask_Status=true;
      Serial.println("Auto-Cancel task has been enabled! Meeting will be canceled soon!"); 
     }     
  }

}

void readCardIfAvailable(){
    if (RC522.isCard()){
    AuthCardID ="";
    /* If so then get its serial number */
    RC522.readCardSerial();
    Serial.println("Card detected:");
    for(int i=0;i<5;i++)
    {
      //Serial.print(RC522.serNum[i],DEC);
      //Serial.print(RC522.serNum[i],HEX); //to print card detail in Hexa Decimal format
 
      AuthCardID= String(AuthCardID + RC522.serNum[i]);
    }
    Serial.print("Following card is booking the meeting room: ");
    Serial.println(AuthCardID);
    
    if(AuthCardID.equals(ID))
   //if(CheckCardID(AuthCardID))
    {
      RoomAvailability=false;
      currentExtendCount=1;
      //Time stamp is stored when the booking is performed in order to determine remaining time.
      bookingTimeStamp=millis()/1000;
      Serial.println("Detected card matches the one defined! Room is bussy now!");

     }else{
      Serial.println("The detected card does not match the one defined!Room is still free!");
      }
  }
}

/*********************************************
 Methods used when a meeting room is occupied.
**********************************************/
void calculateRemainingTime(){
   int currentSecond=millis()/1000;
   //Stores how  much time passed since the  meeting room was booked
   int timeSinceBooking= currentSecond - bookingTimeStamp;
   remainingTime = BOCKING_PERIOD* 60 * currentExtendCount - timeSinceBooking;
}
void printRemainingBookingTime(){

   //remainingTime = BOCKING_PERIOD - timeSinceBooking;
   lcd.clear();
   lcd.setCursor(0,0);     
   lcd.print("Room is occupied");
      
   Serial.print("Remaining time:");
   Serial.println(remainingTime);
    
   lcd.setCursor(0,1);
   lcd.print("                ");
   lcd.setCursor(0,1);
   char text[16]; 
   sprintf(text,"Wait for %d:%d",remainingTime/60 , remainingTime%60);

   lcd.print(text);
  }

void autoCancelMeetingTask(){
  RoomAvailability=true;
  Serial.println("Meeting has been canceled since all attendts have left!");
  Alarm.disable(autoCancelTask);
  autoCancelTask_Status=false;
 }
  
/*Method used to track the the number of persons that enter/exit the meeting room*/
void trackAttendants(){
  int firstSensor = analogRead(IR_ANALOG_1);
//  Serial.print("Analog sensor 1: ");
//  Serial.println(firstSensor);
  if(firstSensor > IR_THRESHOLD){
     firstSensorTriggered=true;
     //Serial.println("First IR sensor triggered!");
    if(sensorTriggerDirection == NO_IR_SENSOR_TRIGGERED){
      sensorTriggerDirection =IR1_TRIGGERED_FIRST;
      }

    }
    
  int secondSensor = analogRead(IR_ANALOG_2);
//  Serial.print("Analog sensor 1: ");
//  Serial.println(firstSensor);
  if(secondSensor > IR_THRESHOLD){
    secondSensorTriggered=true;
   // Serial.println("Second IR sensor triggered!");
    
    if(sensorTriggerDirection == NO_IR_SENSOR_TRIGGERED){
      sensorTriggerDirection = IR2_TRIGGERED_FIRST;
     }
     
    }
    
  if(firstSensorTriggered && secondSensorTriggered){

      if(sensorTriggerDirection==IR1_TRIGGERED_FIRST){
        numberOfPersons++;
        Serial.print("Number of persons increased: ");
        Serial.println(numberOfPersons);
      }else if(sensorTriggerDirection == IR2_TRIGGERED_FIRST){
          if(numberOfPersons>0){
            numberOfPersons--;
            Serial.print("Number of persons decreased: ");
            Serial.println(numberOfPersons);
            }
       }
      sensorTriggerDirection= NO_IR_SENSOR_TRIGGERED;
      firstSensorTriggered=false;
      secondSensorTriggered=false;
    }
 }


void sendFreeMeetingRoomDataToServer(){
  int temperature=readTemperature();
  // AddData(1,"NA","NA", temperature);
  Serial.println("Sending data to server in free meeting room behavior.");
}
void sendOccupiedMeetingRoomDataToServer(){
  // AddData(1,"NA",,"NA", temperature);
   Serial.println("Sending data to server for occupied meeting room behavior.");
}

/*
 * Method used to read the temperature as double result.
*/
int readTemperature(){
  int adcValue=analogRead(TEMPERATURE_ADC);
  float temperature= adcValue* TEMPERATURE_COEF_PER_ADC;
  return temperature;
}

/***********************************
* LCD Keypad related functionalities
************************************/
void handleMenuNagivation(){
 uint8_t button= lcd.readButtons();

 if(currentView == MAIN_VIEW){
  if(button == BUTTON_SELECT){
    currentView=MENU_ITEM_EXPAND;
    }
 }else if(currentView == MENU_ITEM_EXIT){
    if(button== BUTTON_SELECT){
      currentView = MAIN_VIEW;
     }
 }else if(currentView == MENU_ITEM_EXPAND){
    if(button == BUTTON_SELECT){
       extendMeetingIndicator=currentExtendCount;
       currentView = MENU_ITEM_EXPAND_SETTING;
      }else if(button == BUTTON_DOWN || button == BUTTON_UP){
        currentView = MENU_ITEM_EXIT;
      }
 }else if(currentView == MENU_ITEM_EXPAND_SETTING){
     if(button == BUTTON_SELECT){
      currentExtendCount=extendMeetingIndicator;
       currentView = MENU_ITEM_EXPAND;
      }else if (button == BUTTON_RIGHT 
                 && extendMeetingIndicator < MAX_EXTEND_COUNT){
        extendMeetingIndicator++;
       }else if(button == BUTTON_LEFT 
                && extendMeetingIndicator > MIN_EXTEND_COUNT){
                  extendMeetingIndicator--;
        }
 }
 }
 void renderView(){
  if(currentView == MAIN_VIEW){
    if(RoomAvailability==true){
      lcd.clear();
      lcd.setCursor(0,0);     
      lcd.print("Room is free");
     }else{
      if(numberOfPersons>0){
         printRemainingBookingTime();
        }else{
        lcd.clear();
        lcd.setCursor(0,0);     
        lcd.print("Will be canceled");
        }    
     }
        
  }else if(currentView==MENU_ITEM_EXPAND){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Extend meeting");  
    }else if (currentView == MENU_ITEM_EXPAND_SETTING){
      lcd.clear();
      lcd.setCursor(0,0);
      String value= "Duration:";
      value=value+extendMeetingIndicator;
      value=value+"/4";
      lcd.print(value);
      lcd.setCursor(0,1);
      lcd.print("-              +");
      
   }else if (currentView == MENU_ITEM_EXIT){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Exit");
  }
  }


/***********************************
* Ethernet related functionalities 
************************************/

boolean CheckCardID(String CardID){
  client.connect(server, 80);

String value=String("cardID="+CardID);
   String response="";
  if (client.connected()) {
    client.println("POST /MeetingRoomTracker/MeetingRoomTracker.asmx/ValidateUser HTTP/1.1");
    client.println("Host: localhost"); 
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: "); client.println(value.length());   
    client.println();
    client.println(value);  
    client.println(); 
    delay(1000);
    
    while (client.available()) 
    { 
      char c = client.read();
      response= String(response+c);
      Serial.print(c);
    }
  }
  client.stop();
  client.flush();
 return response.indexOf("true")>0;
}

void AddData(String teamId,String CardID,String numberOfParticipants,String temperature){
  client.connect(server, 80);

 String teamValue=String("teamId="+teamId);
 String cardValue=String("&cardId="+CardID);
 String participantsValue=String("&numberOfParticipants="+numberOfParticipants);
 String temperatureValue=String("&temperature="+temperature);
  String content= String(teamValue+cardValue+participantsValue+temperatureValue);

  if (client.connected()) {
    client.println("POST /MeetingRoomTracker/MeetingRoomTracker.asmx/AddData HTTP/1.1");
    client.println("Host: localhost"); 
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: "); client.println(content.length());   
    client.println();
    client.println(content); 
    client.println();

     Serial.println(content);
    
    delay(1000);

    while (client.available()) 
    { 
      char c = client.read();
      Serial.print(c);
    }
  }
  client.stop();
  client.flush();
}

boolean testConnection(){
  String response="";
  client.connect(server, 80);

  if (client.connected()) {
    client.println("POST /MeetingRoomTracker/MeetingRoomTracker.asmx/TestConnection HTTP/1.1");
    client.println("Host: localhost"); 
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: "); client.println(0);
    
    client.println();
    delay(1000);

    while (client.available()) 
    { 
      char c = client.read();
      response= String(response+c);
    }
  }
  client.stop();
  client.flush();
  //Print HTTP response content for debug purpose.
  //Serial.println(response);
  //Extract only the response if available.
  return response.indexOf("true")>0;
}
