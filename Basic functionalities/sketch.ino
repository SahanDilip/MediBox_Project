#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define NTP_SERVER     "pool.ntp.org"
float UTC_OFFSET  =  (5.5)*3600;
int UTC_OFFSET_DST = 0;

#define BUZZER 15
#define LED_1 5
#define PB_CANCEL 35
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 25
#define DHTPIN 12

#define TEMP_WARN_LED 16
#define HUM_WARN_LED 17

Adafruit_SSD1306 display(SCREEN_WIDTH,SCREEN_HEIGHT,&Wire,OLED_RESET);
DHTesp dhtSensor;

int days;
int hours;
int minutes;
int seconds;

bool alarm_enabled = true ;

int n_alarms = 3;
int alarm_hours[] = {0,1,2};
int alarm_minutes[] = {15,30,45}; 
bool alarm_triggered[] = {false,false,false}; 

int n_notes = 8;
int C = 262, D = 294, E = 330, F = 349, G = 392, A = 440, B = 494, C_H = 523;
int notes[] = {C,D,E,F,G,A,B,C_H};

int current_mode = 0;
int max_modes = 5;
String modes[] = {"1 - Set Time Zone", 
                  "2 - Set Alarm 1" , 
                  "3 - Set Alarm 2" , 
                  "4 - Set Alarm 3" , 
                  "5 - Disable Alarms"};

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//code run once

void setup() {
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);

  pinMode(TEMP_WARN_LED, OUTPUT);
  pinMode(HUM_WARN_LED, OUTPUT);

  dhtSensor.setup(DHTPIN, DHTesp::DHT22);

  Serial.begin(115200);
  // Check whether communication configured correctly.
  if (! display.begin(SSD1306_SWITCHCAPVCC,SCREEN_ADDRESS)){
    Serial.println(F("SSD1306 Allocation Failed"));
    for(;;);
  }
  display.display();
  delay(500);

  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WIFI",0,0,2);
  }

  display.clearDisplay();
  print_line("Connected to WIFI",0,0,2);

  display.clearDisplay();
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);

  display.clearDisplay();
  print_line("Welcome to Medibox!",10,10,2);
  display.clearDisplay();

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//code runs repeatedly

void loop() {
  update_time_with_check_alarm();
  if(digitalRead(PB_OK) == LOW){
    delay(200);
    go_to_menu();
  }
  check_temp();
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//print text in the oled display.

void print_line(String text, int column,int row,int text_size){

  display.setTextSize(text_size); 
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column,row);
  display.println(text);
  display.display();

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//print time

void print_time_now(void){

  display.clearDisplay();
  print_line("Date: " + String(days), 0, 0, 1); 
  print_line("", 0, 30, 2);
  print_line(String(hours), 30, 30, 2);         
  print_line(":", 50, 30, 2);
  print_line(String(minutes), 60, 30, 2);
  print_line(":", 80, 30, 2);
  print_line(String(seconds), 90, 30, 2);
  delay(400);
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//Update time over Wifi

void update_time(){
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  hours = atoi(timeHour);

  char timeMinute[3];
  strftime(timeMinute,3, "%M", &timeinfo);
  minutes = atoi(timeMinute);

  char timeSecond[3];
  strftime(timeSecond,3, "%S", &timeinfo);
  seconds = atoi(timeSecond);

  char timeDay[3];
  strftime(timeDay,3, "%d", &timeinfo);
  days = atoi(timeDay);
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//updates time with check alarm

void update_time_with_check_alarm(void){
  update_time();
  print_time_now();

    if(alarm_enabled == true){
      for(int i = 0 ; i < n_alarms ; i++){
        if(alarm_triggered[i] == false && alarm_hours[i] == hours && alarm_minutes[i] == minutes){
          ring_alarm();
          alarm_triggered[i] = true;
          Serial.println("Alarm Triggered");
        }
      }
    }
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//ringing the alarm

void ring_alarm(){
  display.clearDisplay();
  print_line("MEDICINE TIME!", 0,0,2);
  digitalWrite(LED_1, HIGH);
  bool break_happened = false;
  delay(200);

  while(break_happened == false && digitalRead(PB_CANCEL) == HIGH){
    for(int i = 0 ; i< n_notes ; i++){
      if(digitalRead(PB_CANCEL) == LOW){
        delay(200); //to avoid from the bouncing
        break_happened = true;
        break;
      }
      tone(BUZZER,notes[i]);
      delay(500);
      noTone(BUZZER); //stop the previously played tone
      delay(2);
      
    }
  }

  digitalWrite(LED_1, LOW);
  display.clearDisplay();
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
// Checking the Temperature and humidity

void check_temp() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  bool tempHigh = data.temperature > 32;
  bool tempLow = data.temperature < 26;
  bool humidityHigh = data.humidity > 80;
  bool humidityLow = data.humidity < 60;

  if (tempHigh) {
    display.clearDisplay();
    digitalWrite(TEMP_WARN_LED, HIGH);
    print_line("TEMP HIGH", 0, 40, 1);
    delay(800);
  } else if (tempLow) {
    display.clearDisplay();
    digitalWrite(TEMP_WARN_LED, HIGH);
    print_line("TEMP LOW", 0, 40, 1);
    delay(800);
  } else {
    digitalWrite(TEMP_WARN_LED, LOW);
  }

  if (humidityHigh) {
    display.clearDisplay();
    digitalWrite(HUM_WARN_LED, HIGH);
    print_line("HUMIDITY HIGH", 0, 50, 1);
    delay(800);
  } else if (humidityLow) {
    display.clearDisplay();
    digitalWrite(HUM_WARN_LED, HIGH);
    print_line("HUMIDITY LOW", 0, 50, 1);
    delay(800);
  } else {
    digitalWrite(HUM_WARN_LED, LOW);
  }
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//move menu between modes

void go_to_menu(){
  while(digitalRead(PB_CANCEL) == HIGH){ 
    display.clearDisplay();
    print_line(modes[current_mode],0,0,2);

    int pressed = wait_for_button_press();
    if(pressed == PB_UP){
      delay(200);
      current_mode += 1;
      current_mode = current_mode % max_modes;
    }
    else if(pressed == PB_DOWN){
      delay(200);
      current_mode -= 1;
      if(current_mode < 0){
        current_mode = max_modes - 1;
      }
    }
    else if(pressed == PB_OK){
      delay(200);
      Serial.println(current_mode);
      run_mode(current_mode);
    }
    else if(pressed == PB_CANCEL){
      delay(200);
      break;
    }
  }
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
// running the selected mode

void run_mode(int mode){
  if (mode == 0){
    set_time_zone();
    
  }

  else if (mode == 1 || mode == 2 || mode == 3 ){
    set_alarm(mode);
    alarm_triggered[mode - 1] = false;
    alarm_enabled = true;
    
  }

  else if (mode == 4){
    print_line("Alarms Disabled", 0,0,2);
    delay(200);
    alarm_enabled = false;
  }
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//Set time zone respective to place

void set_time_zone(){
  float new_UTC_OFFSET = 0;
  while (true) {

    display.clearDisplay();
    print_line("Enter UTC offset", 0, 0, 1);
    print_line(String(new_UTC_OFFSET), 0, 10, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      new_UTC_OFFSET += 0.5;                  
    } 
    else if (pressed == PB_DOWN) {
      delay(200);
      new_UTC_OFFSET -= 0.5;                 
    }
    else if (pressed == PB_OK) {
      delay(200);
      UTC_OFFSET = 3600*new_UTC_OFFSET;     
      display.clearDisplay();
      configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
      print_line("Time Zone Set", 0, 0, 2);
      delay(1000);
      break;
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }
} 

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//setting alarm

void set_alarm(int alarm){

  int temp_hour = alarm_hours[alarm-1];
  while (true){
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour),0,0,2);

  int pressed = wait_for_button_press();
    if(pressed == PB_UP){
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    }
    else if(pressed == PB_DOWN){
      delay(200);
      temp_hour -= 1;
      if(temp_hour < 0){
        temp_hour = 23;
      }
    }
    else if(pressed == PB_OK){
      delay(200);
      alarm_hours[alarm-1] = temp_hour;
      break;
    }
    else if(pressed == PB_CANCEL){
      delay(200);
      break;
    }
  }

  int temp_minute = alarm_minutes[alarm-1];
  while (true){
    display.clearDisplay();
    print_line("Enter minutes: " + String(temp_minute),0,0,2);
    int pressed = wait_for_button_press();
    if(pressed == PB_UP){
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    }
    else if(pressed == PB_DOWN){
      delay(200);
      temp_minute -= 1;
      if(temp_minute < 0){
        temp_minute = 59;
      }
    }
    else if(pressed == PB_OK){
      delay(200);
      alarm_minutes[alarm-1] = temp_minute;
      break;
    }
    else if(pressed == PB_CANCEL){
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Alarm is set",0,0,2);  
  delay(200);

}

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//wait until to button press dispay in menu

int wait_for_button_press(){
  while(true){
    if(digitalRead(PB_UP) == LOW){ // normally worked in pull up state
      delay(200);
      return PB_UP;
    }
    else if(digitalRead(PB_DOWN) == LOW){
      delay(200);
      return PB_DOWN;
    }
    else if(digitalRead(PB_OK) == LOW){
      delay(200);
      return PB_OK;
    }
    else if(digitalRead(PB_CANCEL) == LOW){
      delay(200);
      return PB_CANCEL;
    }

    update_time();
  }
}






