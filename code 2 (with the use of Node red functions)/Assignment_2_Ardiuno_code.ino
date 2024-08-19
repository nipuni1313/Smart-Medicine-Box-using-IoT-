//libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <time.h>
#include <ESP32Servo.h>
///-------------------below are the two libraries needed for assignment 2
#include <PubSubClient.h>
#include <WiFi.h>

//for wifi and Mqtt
WiFiClient espClient;//wifi client,  provides wifi signal (ininstance)
PubSubClient mqttClient(espClient);//using the wifi instance,  creates an object named mqttClient of type PubSubClient, which is a library used for MQTT communication.

//LDR
#define LDR_1 36 //right (VP)
#define LDR_2 39//left (VN)

//servo motor
#define motorPin 17 //servo motor

//for the motorangle calculations
String msg ;
float motor_angle_offsets;
float control_factor = 0.75;//controlling factor
float min_angle =30;//minimum angle 
float motor_angle = 0;
float D_servo ;


//needed to calculate light intensity
float GAMMA = 0.75; 
const float RL10 = 50;
float MIN_ANGLE = 30;
float LUX1 = 0;
float LUX2 =0;

String temp_value;//needed to save tempreature values

//character arrays needed for publishing data to mqtt server 
char LDR_ar[6];
char temp_ar[6];
char high_ldr[38];

//objects
Servo servo;// setup servor motor

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
#define LED_2 32//tempreature
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
  Serial.begin(115200);
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
    servo.attach(motorPin, 500, 2400); //servo mototor initializing
    display.display();
    delay(2000);


    connect_to_WiFi();//connect to wifi
    setupMqtt(); //setting up mqtt

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

  //checking whether connected to mqtt client
  if(!mqttClient.connected()){
    Serial.println("inside calling connecttobroker");  
   conect_to_broker();//if not connected, connect to mqtt using connect_to_broker() function
  }

    mqttClient.loop(); // ensure that the MQTT client continuously processes any incoming messages, publishes any outgoing messages, and maintains communication with the MQTT broker.

    //publishes any available values while looping
    mqttClient.publish("LDR value",LDR_ar);
    mqttClient.publish("LDR high",high_ldr);
    mqttClient.publish("Tempreature",temp_ar);

    delay(1000);

    temp_change_to_char();
    light_intensity();
    sliding_window();


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

//function for establishing mqtt connection
void setupMqtt(){
  mqttClient.setServer("test.mosquitto.org",1883);//setup mqtt server, "test.mosquitto.org"
  mqttClient.setCallback(receiveCallback);//after subscribing, reciver call back recieves values

}

// function to attempt connection to MQTT broker and subscribes to topics
void conect_to_broker() {  

  //check whether connected, and while it is not connected.....                                             
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");//....print connecting
    if (mqttClient.connect("ESP-8266-67895684567")) {//ESP-8266-67895684567 is the id
      Serial.print("connected");// srver connection success
    
      //subscribe to the below topics of Node red
      mqttClient.subscribe("control_factor");
      mqttClient.subscribe("minimum angle");//best place to subscribe (incoming) is in connect to broker 
    } 

    //....if mqtt connection failed
    else {
      Serial.print("failed ");
      Serial.print(mqttClient.state());//mqtt client state gives what the problem in connection is
      delay(5000);//wait 5s before attempting reconnection
    }
  }
}

//function for creating and selecting the maximum light inensity values
void light_intensity(){
  calculate_LDR_values();//calculte LUX1 & LUX2
  String str_temp;

  //checking if intensity values are nan and if so, setting them to zero
  //if nan, we can not compare
  if(isnan(LUX1)){
    LUX1 = 0;
  }
  if(isnan(LUX2)){
    LUX2 =0;
  }

  //comparing maximum light intensity values
    //right ldr is higher
  if(LUX1>LUX2){
    String(LUX1).toCharArray(LDR_ar,6);
    str_temp = "Right LDR gives the highest intensity";
    str_temp.toCharArray(high_ldr,38);
  }
    //left ldr is higher
  else if(LUX2>LUX1){
    String(LUX2).toCharArray(LDR_ar,6);
    str_temp = "Left LDR gives the highest intensity";
    str_temp.toCharArray(high_ldr,38);
  }
    //both ldrs is have same value 
  else{
    str_temp = "both LDRs give the same intensity";
    str_temp.toCharArray(high_ldr,38);
  }
}


//funtion to calculate motor angle
void sliding_window(){
  calculate_LDR_values();//calculte LUX1 & LUX2
  if(isnan(LUX1)){
    LUX1 = 0;
   }
  if(isnan(LUX2)){
     LUX2 =0;
  }

  if(LUX2 < LUX1){
    D_servo = 1.5;//given in assignment
  }
  else if(LUX2 > LUX1){
    D_servo= 0.5;//given in assignment
  }

  float max_intensity = max(LUX1,LUX2);//take the maximum of the 2 intensities as the maximum light intensity
  if(min_angle!=0 && control_factor != 0 )
  {
    motor_angle_offsets = min_angle;
    motor_angle = min((motor_angle_offsets*D_servo) + (180 - motor_angle_offsets)*max_intensity*control_factor,(float)180) ;//calculate motor angel using the equation
    Serial.print("motor_angle: ");
    Serial.println(motor_angle);
    servo.write(motor_angle);//set the servo motor's angel to the calculated value
    delay(15);
  }
}

//function to calculate maximum light intensities in 0-1 range
void calculate_LDR_values(){

  float volt_1 = analogRead(LDR_1) / 1024. * 5;
  float R1 = 2000 * volt_1 / (1 - volt_1 / 5);
  float max_LUX1 = pow(RL10 * 1e3 * pow(10, GAMMA) / 322.58, (1 / GAMMA));// max intensity of light of right ldr
  LUX1 = pow(RL10 * 1e3 * pow(10, GAMMA) / R1, (1 / GAMMA))/max_LUX1;//max light intensity (R) range 0-1


  float volt_2 = analogRead(LDR_2) / 1024. * 5;
  float R2 = 2000 * volt_2 / (1 - volt_2 / 5);
  float max_LUX2 = pow(RL10 * 1e3 * pow(10, GAMMA) / 322.58, (1 / GAMMA));//calculate max intensity of light of left ldr
  LUX2 = pow(RL10 * 1e3 * pow(10, GAMMA) / R2, (1 / GAMMA))/max_LUX2;//max light intensity (L) range 0-1
}

//function to convert temreature values to character array
void temp_change_to_char(void){
  TempAndHumidity data = dhtSensor.getTempAndHumidity();//get temp and humid values
  delay(100);
  temp_value = String(data.temperature, 2);//convert tempreature value of 2 decimal places to string 
  temp_value.toCharArray(temp_ar,6);//convert string into char array
}

//Receiving data from the server
void receiveCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("message arrived [");
  Serial.print(topic);//the topic of the rxed msg will display
  Serial.print("] ");

  char payloadCharAr[length];//the payload that was recived, saved in payloadcharar, with the coming data size
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    payloadCharAr[i] = (char)payload[i]; //convert each incoming data payload into character
  }
  Serial.println();//print the recived data

  String str_msg;//create a string variable

  //if the recived message topic is "minimum angle"...
  if (strcmp(topic,"minimum angle")==0) {
    for (int i = 0; i < length; i++) {
      str_msg += (char)payload[i];//create the string str_msg by concatinating each charcter of payload
      min_angle = str_msg.toFloat();//generate minimum angle floating point value, and set it to min_angle variable
      delay(100); // Add delay for stabilization
    }
     Serial.print("Min angle: ");
     Serial.println(min_angle);  
  }

  
  //if incoming topic is "control_factor"...
  if (strcmp(topic,"control_factor")==0) {
    for (int i = 0; i < length; i++) {
      str_msg += (char)payload[i];//create the string str_msg by concatinating each charcter of payload
      control_factor = str_msg.toFloat();//generate control factor floating point value, and set it to control_factor variable
      delay(100); // Add delay for stabilization
    }
    Serial.print("Control factor: ");
    Serial.println(control_factor);
  }
}



//------------------------------ASSIGNMENT 2 CODES ARE OVER------


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