#include <WiFi.h>
#include <FirebaseESP32.h>
#include <TimeLib.h>
#include "RTClib.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Fingerprint.h>
#include  <hardwareserial.h>
#define mySerial Serial2
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
#define RELAY 4
//firebase
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#define WIFI_SSID "aoa"
#define WIFI_PASSWORD "nikoajalo"
#define FIREBASE_HOST "https://penyimpanan-ta-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "AIzaSyDwOnNlL_pNRB30pQA57b6kbtBZGI_aEsY"
FirebaseData firebaseData;
FirebaseData fbdo;
tmElements_t my_time;  // time elements structure
time_t unix_timestamp; // a timestamp
RTC_DS1307 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);
String d = "Disita";


int x,y,z;      //X Y adalah nilai untuk menghitung waktu lebih dalam sekala jam, Z adalah nilai akhir
int h,b,s;      // H adalah hari, B adalah batasan waktu yang berlebih, S adalah nominal biaya

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void setup () {
  Serial.begin(9600); 

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    
//LCD 16x2
  lcd.begin (); // initialize the lcd
  lcd.backlight (); // To power on teh backlight
  
      //finggerprint
  while (!Serial);  
  delay(100);
  Serial.println("\n\nAdafruit finger detect test");
  finger.begin(57600);
  
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else {
    Serial.println("Waiting for valid finger...");
      Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  }
  
//relay finggerprint
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);
  
//Rtc
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop () 
{
  
 getFingerprintID();
  //lama sewa
  int bsewa = 1;
  // batas sewa
  int year = 2022;
  int month = 2;
  int day = 2;
  int hour = 0;
  int minute = 0;
  int second = 0;

   Firebase.setInt(fbdo,"/Sensor/TahunSewa", year);
    Firebase.setInt(fbdo,"/Sensor/BulanSewa", month);
     Firebase.setInt(fbdo,"/Sensor/HariSewa", day);

  // Convert human-readable date to Unix timestamp
  tmElements_t tm;
  tm.Year = year - 1970;   // Years since 1970
  tm.Month = month;
  tm.Day = day;
  tm.Hour = hour;
  tm.Minute = minute;
  tm.Second = second;
  time_t unixTimesewa = makeTime(tm);

  // Set the converted Unix timestamp
  setTime(unixTimesewa);

  //RTC
    DateTime now = rtc.now();
    lcd.setCursor(0, 0);
    lcd.print("DATE");
    lcd.print(" ");
    lcd.print(now.day(), DEC);
    lcd.print('/');
    lcd.print(now.month(), DEC);
    lcd.print('/');
    lcd.print(now.year(), DEC);
    lcd.print(" ");
    //lcd.print(daysOfTheWeek[now.dayOfTheWeek()]);
    //lcd.print(" ");
 
    lcd.setCursor(0, 1);
    lcd.print(now.hour(), DEC);
    lcd.print(':');
    lcd.print(now.minute(), DEC);
    lcd.print(':');
    lcd.print(now.second(), DEC);
    lcd.println("                 ");

    Firebase.setInt(fbdo,"/Sensor/Tahun", now.year());
    Firebase.setInt(fbdo,"/Sensor/Bulan", now.month());
    Firebase.setInt(fbdo,"/Sensor/Hari", now.day());
     
//tetapan waktu sewa
 s=18000*bsewa; //harga sewa 
  Firebase.setInt(fbdo,"/BiayaSewa", s);
 h=(1000*86400); // 86400 detik adalah 24jam waktu dengan batasan keterlambatan

//permisalan waktu sewa melebihi batas yang di tentukan
if (unixTimesewa < now.unixtime()){
        for (x=0 ; x<=h ; x++){         
      if(x%3600000 == 0){ //3600000 pembagi perjam
         
       y=(x/3600000)*10000; //(x/3600000 adalah pembagian waktu perjam dan 10000 adalh biaya yang di kenakan tiap ketertlabatan 1 jam)
       z=y+s;
        Firebase.setInt(fbdo,"/BiayaSewa", z);
        finger.getImage();
       //skrng
       Serial.print("tahun : ");
       Serial.println(now.year());
       Serial.print("bulan : ");
       Serial.println(now.month());
       Serial.print("hari : ");
       Serial.println(now.day());
       //sewa
       Serial.print("tahuns : ");
       Serial.println(year);
       Serial.print("bulans : ");
       Serial.println(month);
       Serial.print("haris : ");
       Serial.println(day);
       Serial.print("biaya sewa : ");
       Serial.println(z);
       delay (2000); // perubahan nilai denda setiap jam
       
      }
    
       if (x>=h){
      Serial.println(d);
       Firebase.setString(fbdo,"/BiayaSewa", d);
      delay(10000);
      }
       
        }
}                 


      else{
          getFingerprintID();
             //skrng
             Serial.print("tahun : ");
             Serial.println(now.year());
             Serial.print("bulan : ");
             Serial.println(now.month());
             Serial.print("hari : ");
             Serial.println(now.day());
             //sewa
             Serial.print("tahuns : ");
             Serial.println(year);
             Serial.print("bulans : ");
             Serial.println(month);
             Serial.print("haris : ");
             Serial.println(day);
             Serial.print("biaya sewa : ");
             Serial.println(s);
             
             
             delay (1000); 
            }


}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
    digitalWrite(RELAY, LOW); // unlock the door
    delay(5000);
    digitalWrite(RELAY, HIGH);// lock the door
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}
