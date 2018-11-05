#include <SPI.h>
#include <Ethernet.h>
#include <RFID.h>
 
#include <LiquidCrystal.h>

#define SDA_DIO 9
#define RESET_DIO 8
/* Create an instance of the RFID library */
RFID RC522(SDA_DIO, RESET_DIO); 

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
  delay(2000);
}
String AuthCardID;

boolean ServerAuthorization = false;
boolean RoomAvailability = true;
String RoomState ="";

int bookingTimeStamp;
int remainingTime;

int bookTime = 0;
void loop()
{ 
  if( RoomAvailability == true)
  {
    
    RoomState = "FREE";
  }
  else
  {
    remainingTime = bookTime*60 - (millis()/1000);
    RoomState = "BUSY";
  }
//  lcd.setCursor(0,0);    
  
 // lcd.print("Room is "+RoomState);

  if( RoomAvailability == false)
  {
 //   lcd.setCursor(0,1);
  //  lcd.print("                ");
 //    lcd.setCursor(0,1);
    char text[16]; 
    sprintf(text,"Wait for %d:%d",remainingTime/60,remainingTime%60);
 //   lcd.print(text);
    
  }
  if(remainingTime == 0)
  {
    RoomAvailability = true;
  }
  
  if (RC522.isCard())
  {
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
      boolean result= CheckCardID(AuthCardID);
      ServerAuthorization = result;
      Serial.print("Result of checkIf is : ");
      Serial.println(result);
      Serial.println(AuthCardID);

      bookTime = 15;
      String bookText = "";
      do
      {
        lcd_key = read_LCD_buttons();   // read the buttons
        switch(lcd_key)
        {
           case btnUP:
           {
              bookTime += 15;
              if( bookTime > 60)
              {
                bookTime = 60;
              }
              break;
           }
           case btnDOWN:
           {
              bookTime -= 15;
              
              if( bookTime <= 15)
              {
               bookTime = 15;
              } 
              break;
           }
        }

      //  lcd.clear();
      //  lcd.setCursor(0,0);  
        bookText = String(bookTime);
        
      //  lcd.print(bookText);
        delay(400);
      }
      while(lcd_key !=btnSELECT);
     RoomAvailability = false; 
  //   AddData("1",AuthCardID,"1","15");
     bookingTimeStamp = millis()/1000;
     remainingTime+=bookingTimeStamp;
  }
  
   
}



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
