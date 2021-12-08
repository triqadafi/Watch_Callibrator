#include <Arduino.h>
#include "DS1307.h"
#include "RTClib.h"
#include <SoftwareSerial.h>


#define BLUETOOTH_SERIAL_TX 2
#define BLUETOOTH_SERIAL_RX 3

SoftwareSerial BluetoothSerial(BLUETOOTH_SERIAL_RX, BLUETOOTH_SERIAL_TX); // RX, TX


// HIGH LEVEL
RTC_DS1307 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
DateTime RTC_now;

// LOW LEVEL
DS1307 clock;//define a object of DS1307 class
void printTime();
bool getTime(const char *str);
bool getDate(const char *str);

  int FI_HOUR, FI_MIN, FI_SEC;
  int FI_DAY, FI_MONTH, FI_YEAR;

unsigned long MAIN_millis = 0;
int MAIN_state = 0;
unsigned long STATE_A_millis = 0;
unsigned long STATE_B_millis = 0;

/* ---------------------------------------------
 * @ USART VARIABLE
 * -------------------------------------------*/
char FI_BUFFER[72] = {0};              // a string to hold incoming data
bool FI_USART_RECEIVER_FLAG = false;   // whether the string is complete
uint8_t FI_USART_BUFFER_INDEX = 0;

void BluetoothserialEvent();
void fi_usart_update(char* arr);

void setup()
{
	Serial.begin(115200);
  // while (!Serial); // wait for serial port to connect. Needed for Native USB only
	BluetoothSerial.begin(9600);

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

  // get the date and time the compiler was run
  // if (getDate(__DATE__) && getTime(__TIME__)) {
  //   Serial.print(FI_HOUR, DEC);
  //   Serial.print(":");
  //   Serial.print(FI_MIN, DEC);
  //   Serial.print(":");
  //   Serial.print(FI_SEC, DEC);
  //   Serial.print("	");
  //   Serial.print(FI_DAY, DEC);
  //   Serial.print("/");
  //   Serial.print(FI_MONTH, DEC);
  //   Serial.print("/");
  //   Serial.print(FI_YEAR, DEC);
  //   Serial.println();
    
  // }



  pinMode(LED_BUILTIN, OUTPUT);
  MAIN_state = 1;
  RTC_now = rtc.now();

}

void loop() {
  if (millis() - MAIN_millis > 1000){
    MAIN_millis = millis();

    RTC_now = rtc.now();

    if(analogRead(A0) > 600){
      Serial.print("=============== WATCH UPDATED ===============");

      clock.begin();
      clock.fillByYMD(RTC_now.year(), RTC_now.month(), RTC_now.day());//Jan 19,2013
      clock.fillByHMS(RTC_now.hour(), RTC_now.minute(), RTC_now.second());//15:28 30"
      clock.setTime();//write time to the RTC chip
      printTime();
    }
  }
  if (MAIN_state == 0) {
  }else if (MAIN_state == 1) {
    if (millis() - STATE_A_millis > 1000){
      Serial.print("DATE: ");
      Serial.print(RTC_now.year(), DEC);
      Serial.print('/');
      Serial.print(RTC_now.month(), DEC);
      Serial.print('/');
      Serial.print(RTC_now.day(), DEC);
      Serial.print(" (");
      Serial.print(daysOfTheWeek[RTC_now.dayOfTheWeek()]);
      Serial.print(") ");
      Serial.println();
      Serial.print("TIME: ");
      Serial.print(RTC_now.hour(), DEC);
      Serial.print(':');
      Serial.print(RTC_now.minute(), DEC);
      Serial.print(':');
      Serial.print(RTC_now.second(), DEC);
      Serial.println();

      Serial.print("A0: ");
      Serial.println(analogRead(A0));

      MAIN_state = 2;
      STATE_A_millis = millis();
      STATE_B_millis = millis();
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }else if (MAIN_state == 2) {
    if (millis() - STATE_B_millis > 200){
      digitalWrite(LED_BUILTIN, LOW);
      MAIN_state = 1;
    }
  }

  BluetoothserialEvent();
  // Where are you going? Please stay here! >_<

}
/*Function: Display time on the serial monitor*/
void printTime()
{
	clock.getTime();
	Serial.print(clock.hour, DEC);
	Serial.print(":");
	Serial.print(clock.minute, DEC);
	Serial.print(":");
	Serial.print(clock.second, DEC);
	Serial.print("	");
	Serial.print(clock.month, DEC);
	Serial.print("/");
	Serial.print(clock.dayOfMonth, DEC);
	Serial.print("/");
	Serial.print(clock.year+2000, DEC);
	Serial.println();
}


const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
bool getTime(const char *str){
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  FI_HOUR = Hour;
  FI_MIN = Min;
  FI_SEC = Sec;
  return true;
}
bool getDate(const char *str){
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  FI_DAY = Day;
  FI_MONTH = monthIndex + 1;
  FI_YEAR = Year;
  return true;
}

/*
  SerialEvent occurs whenever a new data comes in the
  hardware serial RX.  This routine is run between each
  time loop() runs, so using delay inside loop can delay
  response.  Multiple bytes of data may be available.
 */

void BluetoothserialEvent() {
  while (BluetoothSerial.available()) {
    // get the new byte:
    char inChar = (char)BluetoothSerial.read();
    Serial.print(inChar);
    // add it to the inputString:
    FI_BUFFER[FI_USART_BUFFER_INDEX] = inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      FI_BUFFER[FI_USART_BUFFER_INDEX+1] = '\0';
      FI_USART_BUFFER_INDEX=0;
      if(strncmp(FI_BUFFER, "3913", 4) == 0)
        FI_USART_RECEIVER_FLAG = true;
    }else{
      FI_USART_BUFFER_INDEX ++;
    }
  }
  
  
  if (FI_USART_RECEIVER_FLAG) {
    FI_USART_RECEIVER_FLAG = false;
    //EXECUTE YOUR COMMAND HERE
    fi_usart_update(FI_BUFFER);
  }
}

// 3913;FI+ADJUST=2019;6;1;18;09;59;
// 3913;FI+ADJUST=2020;12;8;14;16;00;
// =====================================================================
void fi_usart_update(char* arr){
  char * item = strtok (arr, ";"); //getting first word (uses space & comma as delimeter)
  item = strtok (NULL, "="); //getting subsequence word
  uint8_t index = 0;
  if(!(strcmp(item,"FI+ADJUST"))){
    digitalWrite(13, HIGH);
    item = strtok (NULL, ";");
    uint16_t _year = atoi(item);
    item = strtok (NULL, ";");
    uint8_t _month = atoi(item);
    item = strtok (NULL, ";");
    uint8_t _day = atoi(item);
    item = strtok (NULL, ";");
    uint8_t _hour = atoi(item);
    item = strtok (NULL, ";");
    uint8_t _minute = atoi(item);
    item = strtok (NULL, ";");
    uint8_t _second = atoi(item);

// #ifdef FI_DEBUG
    Serial.print(_year); Serial.print(",");
    Serial.print(_month); Serial.print(",");
    Serial.print(_day); Serial.print(",");
    Serial.print(_hour); Serial.print(",");
    Serial.print(_minute); Serial.print(",");
    Serial.print(_second); Serial.print(",");
// #endif
    // January 21, 2014 at 3am you would call:
    rtc.adjust(DateTime(_year, _month, _day, _hour , _minute, _second));
    digitalWrite(13, LOW);
    BluetoothSerial.println("3913;9999;");
  }else if(!(strcmp(item,"FI+NOW"))){
    //3913;FI+NOW=0;
    digitalWrite(13, HIGH);
    BluetoothSerial.print(RTC_now.year(), DEC);
    BluetoothSerial.print('/');
    BluetoothSerial.print(RTC_now.month(), DEC);
    BluetoothSerial.print('/');
    BluetoothSerial.print(RTC_now.day(), DEC);
    BluetoothSerial.print(" ");
    BluetoothSerial.print(RTC_now.hour(), DEC);
    BluetoothSerial.print(':');
    BluetoothSerial.print(RTC_now.minute(), DEC);
    BluetoothSerial.print(':');
    BluetoothSerial.print(RTC_now.second(), DEC);
    BluetoothSerial.println();
    digitalWrite(13, LOW);
  }else if(!(strcmp(item,"FI+PING"))){
    BluetoothSerial.println("PING!"); 
  }

}

