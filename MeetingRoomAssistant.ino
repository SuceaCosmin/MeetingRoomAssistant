#include <SPI.h>
#include <Ethernet.h>
#include <RFID.h>
 
#include <LiquidCrystal.h>

#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>


#define WHITE 0x7
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


#define TEMPERATURE_ADC_PIN A3

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

//LCD keypad menu defines
#define TIMER_SCREEN  0
#define EXTEND_MEETING_SETING_ITEM 10
#define EXTEND_MEETING_SETING 20

/*Variable used to store to current view displayed on LCD*/
int lcdView=0;


//Ethernet shield initialization
byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x0E, 0x03, 0xE2};
IPAddress ip(192,168,0,251);
byte server[] = {192, 168,   0,  3};  
IPAddress myDns(192,168,0,1);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);
EthernetClient client;

unsigned long lastUpdate = millis();

void setup(){
  Serial.begin(9600); 
  Ethernet.begin(mac, ip, dns, gateway, subnet); 
  RC522.init();
  
  lcd.begin(16, 2);               // start the library
  lcd.setCursor(0,0);             // set the LCD cursor position  
  lcd.setBacklight(WHITE);
  lcd.print("Hello, world!");
  delay(2000);
}
String AuthCardID;

boolean ServerAuthorization = false;
boolean RoomAvailability = true;
String RoomState ="";

int bookingTimeStamp;
int remainingTime;

int BOCKING_PERIOD = 1;
void loop()
{   
delay(500);
//trackAttendants();
 if( RoomAvailability == true){
    readCardIfAvailable();
  }

  if( RoomAvailability == true){
    RoomState = "FREE";
    freeMeetingRoomBehavior();
   Serial.println("Meeting room status is free!");
  }else{
    int currentSecond=millis()/1000;
    //Stores how  much time passed since the  meeting room was booked
    int timeSinceBooking= currentSecond-bookingTimeStamp;
    remainingTime = BOCKING_PERIOD*60 - timeSinceBooking;
    Serial.print("Remaining time:");
    Serial.println(remainingTime);
    RoomState = "BUSY";
    Serial.println("Meeting room status changed to  bussy!");
  }
  
  lcd.setCursor(0,0);     
  lcd.print("Room is "+RoomState);

  if( RoomAvailability == false)
  {
    lcd.setCursor(0,1);
    lcd.print("                ");
    lcd.setCursor(0,1);
    char text[16]; 
    sprintf(text,"Wait for %d:%d",remainingTime/60,remainingTime%60);

    lcd.print(text);
    
  }
  if(remainingTime == 0)
  {
    RoomAvailability = true;
  }
   
}

void freeMeetingRoomBehavior(){
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
 
      AuthCardID= String(AuthCardID+RC522.serNum[i]);
    }
    Serial.print("Following card is booking the meeting room: ");
    Serial.println(AuthCardID);
    
    if(AuthCardID.equals(ID)){
      RoomAvailability=false;
      //Time stamp is stored when the booking is performed in order to determine remaining time.
      bookingTimeStamp=millis()/1000;
      Serial.println("Detected card matches the one defined! Room is bussy now!");

     }else{
      Serial.println("The detected card does not match the one defined!Room is still free!");
      }
  }
}
/*Method used to track the the number of persons that enter/exit the meeting room*/
void trackAttendants(){
  int firstSensor = analogRead(IR_ANALOG_1);
//  Serial.print("Analog sensor 1: ");
//  Serial.println(firstSensor);
  if(firstSensor > IR_THRESHOLD){
     firstSensorTriggered=true;
     Serial.println("First IR sensor triggered!");
    if(sensorTriggerDirection == NO_IR_SENSOR_TRIGGERED){
      sensorTriggerDirection =IR1_TRIGGERED_FIRST;
      }

    }
    
  int secondSensor = analogRead(IR_ANALOG_2);
//  Serial.print("Analog sensor 1: ");
//  Serial.println(firstSensor);
  if(secondSensor > IR_THRESHOLD){
    secondSensorTriggered=true;
    Serial.println("Second IR sensor triggered!");
    
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

int handleMenuNavigation(int buttonPressed){

  if(lcdIsOnScreen(TIMER_SCREEN) && buttonPressed==BUTTON_SELECT){
      lcdView=EXTEND_MEETING_SETING_ITEM;
      return EXTEND_MEETING_SETING_ITEM ;
   }else if(lcdIsOnScreen(EXTEND_MEETING_SETING)){

      if(buttonPressed==BUTTON_RIGHT){
         //Check the incrementation counter that it did not exced max  and increment
       }else if(buttonPressed==BUTTON_LEFT){
         //Check the incrementation counter that it does not read 0 and decrement.
       }else if (buttonPressed==BUTTON_SELECT){
        // Save conviguration and move to to the previeous view.
        lcdView = EXTEND_MEETING_SETING_ITEM;
       }       
   }
   

  return TIMER_SCREEN;
 }


/**
 * Method used to verify if the which view is the LCD display currently at.
 */
boolean lcdIsOnScreen(int value){
  return lcdView==value;
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

void AddData(String value,String CardID,String numberOfParticipants,String temperature){
  client.connect(server, 80);

 String teamValue=String("teamId="+value);
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

void TestConnection(){
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
      Serial.print(c);
    }
  }
  client.stop();
  client.flush();
}
