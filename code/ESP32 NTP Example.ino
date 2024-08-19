//libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

//variables needed for getting time through the wifi
#define NTP_SERVER     "pool.ntp.org" // NTP server address
int UTC_OFFSET=0; // UTC offset in seconds, which is used for time synchronization and adjustments.
#define UTC_OFFSET_DST 0 // Offset during Daylight Saving Time

//for oled
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

//for warnings
#define BUZZER 5
#define LED_1 15//for alarm
#define LED_2 32//temp
#define LED_3 33//humidity

//pushbuttons
#define PB_cancel 25
#define PB_OK 18
#define PB_UP 27
#define PB_DOWN 14

//for humidity and temp sensor
#define DHTPIN 19

//objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor;

//buzzer note variables
int n_notes = 8;
int C = 262, D = 294, E = 330, F = 349, G = 392, A = 440, B = 494, C_H = 523;
int notes[] = {C, D, E, F, G, A, B, C_H};

//for time generation
int days = 0, hours = 0, minutes = 0, seconds = 0;

//for alarm configuration
int user_minutes=0, user_hours=0;
bool alarm_enabled = true;
int n_alarms = 3;
int alarm_hours[] = {0, 0};
int alarm_minutes[]={1,10};
bool alarm_triggered[] = {false, false,false};//three alarms can be implemented


int current_mode = 0, max_modes = 5;
//menue items
String modes[] = {"1- Set Time zone", "2- Set Alarm 1", "3- Set Alarm 2", "4- Set Alarm 3", "5- Disable Alarm"};

void setup() {
    Serial.begin(9600);
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }
    pinMode(BUZZER, OUTPUT);
    pinMode(LED_1, OUTPUT);
    pinMode(LED_2, OUTPUT);
    pinMode(LED_3, OUTPUT);
    pinMode(PB_cancel, INPUT);
    pinMode(PB_OK, INPUT);
    pinMode(PB_UP, INPUT);
    pinMode(PB_DOWN, INPUT);
    dhtSensor.setup(DHTPIN, DHTesp::DHT22);

    display.display();
    delay(2000);

    //connect to wifi
    connect_to_WiFi();

    display.clearDisplay();
    print_line("welcome!", 10, 20, 2);
    delay(2000);
    display.clearDisplay();
}

void loop() {
  //continously run these functions...
    update_time_with_check_alarm();
    if (digitalRead(PB_OK) == LOW) {
        delay(200);
        go_to_menu();
    }
    check_temp_humidity();
}

// Function to connect to WiFi
void connect_to_WiFi() {
  //start wifi connection
    WiFi.begin("Wokwi-GUEST", "", 6);

    //while connecting to wifi
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
        display.clearDisplay();
        print_line("Connecting to wifi",0,0,2);
    }

    //once connected to wifi
    if(WiFi.status() == WL_CONNECTED){
        display.clearDisplay();
        print_line("Connected to wifi",0,0,2);
        configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);//This line configures the device's time settings to synchronize its internal clock with an NTP server.
    }
}

// Function to print text on OLED display
void print_line(String text, int col, int row, int txt_size) {
    display.setTextSize(txt_size);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(col, row);
    display.println(text);
    display.display();
}

// Function to update time from NTP server
void update_time() {
  //This code snippet retrieves the current time information from the system and assigns it to respective variables
  //allowing for time-related functionalities in the program.
    struct tm timeinfo;
    getLocalTime(&timeinfo);//This function retrieves the current local time information and stores it in the timeinfo structure.
    hours = timeinfo.tm_hour;
    minutes = timeinfo.tm_min;
    seconds = timeinfo.tm_sec;
    days = timeinfo.tm_mday;
}

// Function to update time and check for alarms
void update_time_with_check_alarm() {
    update_time();
    print_time_now();
    
    if (alarm_enabled == true)//if alarms are not disabled
     {
        for (int i = 0; i < n_alarms; i++) 
        {//check for each alarm 
            if (alarm_triggered[i] == false && alarm_minutes[i] == minutes && alarm_hours[i]==hours) {
                ring_alarm();
                alarm_triggered[i] = true;
            }
        }
    }
}

// Function to print current time on OLED display
void print_time_now() {
    display.clearDisplay();
    print_line(String(days), 0, 0, 2);
    print_line(":", 20, 0, 2);
    print_line(String(hours), 30, 0, 2);
    print_line(":", 50, 0, 2);
    print_line(String(minutes), 60, 0, 2);
    print_line(":", 80, 0, 2);
    print_line(String(seconds), 90, 0, 2);
    delay(100);
}

// Function to ring the alarm
void ring_alarm() {
    display.clearDisplay();
    print_line("Medicine time!", 0, 0, 2);//display a message on OLED
    digitalWrite(LED_1, HIGH);//switch on LED

    bool break_happened = false;//initialize the break_happened as false
    while (break_happened == false && digitalRead(PB_cancel) == HIGH) {
        for (int i = 0; i < n_notes; i++) {
            if (digitalRead(PB_cancel) == LOW) 
            {
                delay(200);
                break_happened = true;
                break;//if the cancel button is pressed, break from loop
            }
            tone(BUZZER, notes[i]);//play buzzer
            delay(500);
            noTone(BUZZER);
            delay(2);
        }
    }
    digitalWrite(LED_1, LOW);//switch off LED
    display.clearDisplay();
}

// Function to wait for button press
int wait_for_button_press() {
    while (true) {

    //the pressed button will be returned
        if (digitalRead(PB_UP) == LOW) {
            delay(200);
            return PB_UP;
        }
        else if (digitalRead(PB_DOWN) == LOW) {
            delay(200);
            return PB_DOWN;
        }
        else if (digitalRead(PB_OK) == LOW) {
            delay(200);
            return PB_OK;
        }
        else if (digitalRead(PB_cancel) == LOW) {
            delay(200);
            return PB_cancel;
        }
        update_time();
    }
}

// Function to navigate the menu
void go_to_menu() {
    while (digitalRead(PB_cancel) == HIGH)//run until the cancel button is pressed
     {
        display.clearDisplay();
        print_line(modes[current_mode], 0, 0, 2);//print the current mode
        int pressed = wait_for_button_press();
        if (pressed == PB_UP) //navigate up the menue
        {
            delay(200);
            current_mode += 1;
            current_mode = current_mode % max_modes;
        }
        else if (pressed == PB_DOWN) //navigate down the menue
         {
            delay(200);
            current_mode -= 1;
            if(current_mode<0){
                current_mode=max_modes-1;
            }
        }
        else if (pressed == PB_OK)//if OK pressed, run the current mode 
         {
            delay(200);
            run_mode(current_mode);
        }
        else if (pressed == PB_cancel)//if CANCEL is pressed, leave menue
         {
            delay(200);
            break;
        }
    }
}

// Function to set the timezone
//in this function, user can give UTC offset so that the internal clock of the ESP32 can synchronize correctly
void set_time_zone() {
    int entered_hr=0;
    while(true) {
        display.clearDisplay();
        print_line("Enter hour: "+ String(entered_hr),0,0,2);
        //user must enter offset hour

        int pressed = wait_for_button_press();
        if (pressed == PB_UP) {
            delay(200);
            entered_hr+=1;
            entered_hr=entered_hr%15;// offset hour can be changed in the range (14 to -12)
        }
        else if (pressed == PB_DOWN) {
            delay(200);
            entered_hr -= 1;
            if(entered_hr<-12){
                entered_hr=14; // offset hour can be changed in the range (14 to -12)
            }
        }
        else if (pressed == PB_OK) {
            delay(200);
            user_hours=entered_hr;
            break;
        }
        else if (pressed == PB_cancel)//leave set time mode
         {
            delay(200);
            break;
        }
    }

    int entered_min=0;
    while(true) {
        display.clearDisplay();
        print_line("Enter minutes: "+ String(entered_min),0,0,2);
         //user must enter offset hour

        int pressed = wait_for_button_press();
        if (pressed == PB_UP) {
            delay(200);
            entered_min+=1;
            entered_min=entered_min%46; // offset hour can be changed in the range (0 to 45)
        }
        else if (pressed == PB_DOWN) {
            delay(200);
            entered_min -= 1;
            if(entered_min<0){
                entered_min=45;// offset hour can be changed in the range (0 to 45)
            }
        }
        else if (pressed == PB_OK) {
            delay(200);
            user_minutes=entered_min;
            UTC_OFFSET = (user_hours * 3600) + (user_minutes * 60);//redifine UTC_OFFSET with the correct offset for the needed time zone
            configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
            break;
        }
        else if (pressed == PB_cancel)//leave set time zone mode
         {
            delay(200);
            break;
        }
    }
    display.clearDisplay();
    print_line("Time is set",0,0,2);
    delay(1000);
}

// Function to set an alarm
void set_alarm(int alarm) {
    
    int temp_hour=alarm_hours[alarm]; //Sets the alarm time specified by the index 'alarm'.
    while(true) // Loop for setting the alarm hour
    {
        display.clearDisplay();
        print_line("Enter hour: "+ String(temp_hour),0,0,2);
        int pressed = wait_for_button_press();
        if (pressed == PB_UP) {
            delay(200);
            temp_hour+=1;
            temp_hour=temp_hour%24;//hour must not exceed 23, loop back to 0
        }
        else if (pressed == PB_DOWN) {
            delay(200);
            temp_hour -= 1;
            if(temp_hour<0){
                temp_hour=23;// hour loops back to 23 if goes below 0
            }
        }
        else if (pressed == PB_OK) {
            delay(200);
            alarm_hours[alarm]=temp_hour;//in the alarm_hours array update the changed alarm's hour
            break;
        }
        else if (pressed == PB_cancel)
         {
            delay(200);
            break;
        }
    }

    int temp_minute=alarm_minutes[alarm];//Sets the alarm time specified by the index 'alarm'.
    while(true) // Loop for setting the alarm minutes
    {
        display.clearDisplay();
        print_line("Enter minutes: "+ String(temp_minute),0,0,2);
        int pressed = wait_for_button_press();
        if (pressed == PB_UP) {
            delay(200);
            temp_minute+=1;
            temp_minute=temp_minute%60;//hour must not exceed 59, loop back to 0
        }
        else if (pressed == PB_DOWN) {
            delay(200);
            temp_minute -= 1;
            if(temp_minute<0){
                temp_minute=59;// minute loops back to 59 if goes below 0
            }
        }
        else if (pressed == PB_OK) {
            delay(200);
            alarm_minutes[alarm]=temp_minute;//in the alarm_minutes array update the changed alarm's minute
            break;
        }
        else if (pressed == PB_cancel) {
            delay(200);
            break;
        }
    }
    display.clearDisplay();
    print_line("Alarm is set",0,0,2);
    delay(1000);
    display.clearDisplay();
    delay(200);
}

// Function to execute selected menu option
void run_mode(int mode) {
    if(mode==0){
        set_time_zone();//for setting the UTC offset
    }
    else if(mode==1 || mode==2 || mode==3){
        set_alarm(mode-1);// for setting up alarms
    }
    else if(mode==4){
        alarm_enabled=false;//disable all alarms
    }
}

// Function to check temperature and humidity
void check_temp_humidity() {
    TempAndHumidity data = dhtSensor.getTempAndHumidity();
    if (data.temperature > 32) {
        print_line("TEMP HIGH", 0, 40, 1);
        delay(200);
    } else if (data.temperature < 26) {
        print_line("TEMP LOW", 0, 40, 1);
        delay(200);
    }

    if (data.humidity > 80) {
        print_line("HUMIDITY HIGH", 0, 50, 1);
        delay(200);
    } else if (data.humidity < 60) {
        print_line("HUMIDITY LOW", 0, 50, 1);
        delay(200);
    }

    if(60<data.humidity<80 && 26<data.temperature < 32)//if Temp & Humidity are in range, switch off LEDs
    {
        digitalWrite(LED_2, LOW);
        digitalWrite(LED_3, LOW);
    }

    if(data.humidity<60 || data.humidity>80 )//if humidity is out of range, switch ON humidity warning LED
    {
        digitalWrite(LED_3, HIGH);
    }
    if( 32<data.temperature || data.temperature< 26)//if temreature is out of range, switch ON tempreature warning LED
    {
        digitalWrite(LED_2, HIGH);
    }
    delay(200);
    
}