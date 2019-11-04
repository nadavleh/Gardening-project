// This is a working software version which includes a manual mode which allows manual activation of the pump.
// Once the pump is turned off on manual mode, the watering cycle restarts.
// Special "rain on a plant" image is shown in the menu.

#include <LiquidCrystal.h>
//#include <math.h>
LiquidCrystal lcd(6, 7, 5, 4, 3, 2);
byte plantChar[] = { //my special plant character
  B01110,
  B11111,
  B11111,
  B00100,
  B00100,
  B00100,
  B11111,
  B01110
};
byte rainChar[] = {
  B01000,
  B01010,
  B00010,
  B10000,
  B10100,
  B00101,
  B10001,
  B10100
};
//Input & Button Logic
const int numOfInputs = 4; //four input buttons
const int inputPins[numOfInputs] = {8, 9, 10, 11}; // array of 4 with input pins of buttons
int inputState[numOfInputs]; //declare an array for the 4 buttons' state

int lastInputState[numOfInputs] = {LOW,LOW,LOW,LOW};
bool inputFlags[numOfInputs] = {LOW,LOW,LOW,LOW};

//int lastInputState[numOfInputs] = {HIGH, HIGH, HIGH, HIGH};
//bool inputFlags[numOfInputs] = {HIGH, HIGH, HIGH, HIGH};

long lastDebounceTime[numOfInputs] = {0, 0, 0, 0}; // initialize the time array of pressing each button
int debounceDelay = 5; //this delay indicates a butten is presssed over 5 milisecs

//vars nadav added:
const int motor_pin = 12; // the motor pin nadav added to symbolize the pump
bool key_press_flag=0;
int parameter_screen;


unsigned long rest_time=5; // has to be initialized to some value, but it will change in the first loop cycle to values[0][0] 
unsigned long OnTime=0; // we should initialize it to  values[1][0] but im lazy so i do it manually
const unsigned long MinuteInMiliSecs=60000;
const unsigned long HourInMiliSecs=3600000;
const unsigned long SecInMiliSecs=1000;
unsigned long currentMillis = 0;// stores the value of millis() in each iteration of loop()
unsigned long previousMillis = 0;// stores the value of millis() in each iteration of loop()

//LCD Menu Logic
const int numOfScreens = 3; //the menu consists of 2 screens
int currentScreen = 0;  // initialize current screen
int parameters[numOfScreens];
unsigned long parameter_value=0;
String screens[numOfScreens][2] = {
                                    {"once every"   ,   "hours" },
                                    {"open pump for", "minutes"},
                                    {"manual mode", "pump off"}
                                  };

                                   
const unsigned long values[2][8] = {
                                        { 4, 5, 15, ((long)12*3600),  ( (long)24*3600),  ( (long)2*24*3600),  ( (long)3*24*3600), ( (long)7*24*3600) },  // hours to wait in seconds
                                        { 0, 1, 2, 2*60,  3*60,  4*60,  5*60,   10*60  }   // pump on-time in seconds
                                   }; //integer multiplication like 12*3600 will cause an overflow befor assigning the "const unsigned long" and subsequently cause problems. 
                                      //We thus need to cast this multiplications type as long to prevent problems.

//unsigned long parameter_value=values[currentScreen][parameters[currentScreen]]; // this is more reasonable but it doesnt matter as the parameter value get set 
                                                                                  // in the first loop anyways

void setup() {
  lcd.createChar(0, plantChar);
  lcd.createChar(1, rainChar);
//initialize push buttons as pullups
  for (int i = 0; i < numOfInputs; i++) {
    pinMode(inputPins[i], INPUT);
    digitalWrite(inputPins[i], HIGH); // pull-up 20k
  }
//initialize lcd
  lcd.begin(16, 2);
//initialize pump
  pinMode(motor_pin, OUTPUT);//initialize the pump/motor pin as output
  digitalWrite(motor_pin, HIGH); //stupid relay works when pulled low an not high
//initialize serial monitor
  Serial.begin(9600);

 //print initialization to screen:
 //we use type casting: (float) because we divide to integers and unless stated otherwize,
 //serial.print() thinks we send an integer we will get 0 in the print
  Serial.print("initialization of: ");
  Serial.print((double) rest_time/3600,5); // 5 decimal points presicion
  Serial.print(" hours interval at ");
  Serial.print((float) OnTime/((long) 1000*60),5);
  Serial.println(" minutes on-time");
  delay(1000);
}

void loop() {
  unsigned long previous_rest_time=rest_time;
//check if a button has been pressed then released and not just bounced, if so raise a flag
  setInputFlags();
//resolve the raised flag by toggling the screen or selected parameter in a certain screen
  resolveInputFlags();
//check if a parameter value has changed:
  if(key_press_flag==1){
     //Serial.print("press flag: ");
    // Serial.println(key_press_flag);
     
    switch (parameter_screen){
      case 0:
        //the user have changed the pump rest interval
        rest_time = parameter_value;  // i saved the parameter_value as float so i cast it back to long which the initialize works with.
        previous_rest_time=rest_time; // for the manual mode's "pump off" to restart the rest_time after shutting pump down 
        printToSerial();
        break;
      case 1:
        // the user have changed the pump's "on time"
        OnTime= parameter_value*SecInMiliSecs;
        printToSerial();
        break;
      case 2:
        // manual mode on
        if(parameter_value){
              digitalWrite(motor_pin,LOW ); // my stupid relay works with low signal as on   
              screens[currentScreen][1]="pump on";
              rest_time= ((long)1000*3600); // put some ridiculasly long rest_time (like 1000 hours) to disable the pumping loop
        }
        else{
          digitalWrite(motor_pin,HIGH ); 
          screens[currentScreen][1]="pump off";
          rest_time=values[0][parameters[0]];
          lcd.clear();
          lcd.print("watering cycle");
          lcd.setCursor(0, 1);
          lcd.print("will restart ");
          delay(2000);
        }
        printScreen();
        break;        
     }
     
     key_press_flag=0;
     //Serial.print("press flag: ");
     //Serial.println(key_press_flag);
  }
  currentMillis=millis();
  pump();
 // ????????????????????????????????????????????????????????????????
  //Serial.println( (float) currentMillis/1000);
  //Serial.println( (float) previousMillis/1000);

  //Serial.println( (float) currentMillis - previousMillis >= (rest_time*SecInMiliSecs));


 //?????????????????????????????????????????????????????????????????????????
}
void pump()
{
  if ( (currentMillis - previousMillis) >= ((unsigned long) rest_time*SecInMiliSecs)) {
    previousMillis += ((unsigned long) rest_time*SecInMiliSecs);
    digitalWrite(motor_pin,LOW ); // my stupid relay works with low signal as on
    lcd.clear();
    lcd.print("pump on");
    lcd.setCursor(0, 1);
    lcd.print("please wait");
    Serial.println("pump on,please wait");

    delay(OnTime);

    digitalWrite(motor_pin, HIGH);

    printScreen();
    Serial.println("pump off");
  }

}

void setInputFlags() {
  for (int i = 0; i < numOfInputs; i++) {
    int reading = digitalRead(inputPins[i]);
    if (reading != lastInputState[i]) {
      lastDebounceTime[i] = millis();
    }

    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      if (reading != inputState[i]) {
        inputState[i] = reading;
        if (inputState[i] == HIGH) {
          inputFlags[i] = HIGH;
        }
      }
    }
    lastInputState[i] = reading;
  }
}

void resolveInputFlags() {
  for (int i = 0; i < numOfInputs; i++) {
    if (inputFlags[i] == HIGH) {
      inputAction(i);
      inputFlags[i] = LOW;
      printScreen();
    }
  }
}

void inputAction(int input) {
  if (input == 0) {
    if (currentScreen == 0) {
      currentScreen = numOfScreens - 1;
    } else {
      currentScreen--;
    }
  } else if (input == 1) {
    if (currentScreen == numOfScreens - 1) {
      currentScreen = 0;
    } else {
      currentScreen++;
    }
  } else if (input == 2) {
    parameterChange(0);
  } else if (input == 3) {
    parameterChange(1);
  }
}

void parameterChange(int key) {
  //////////////////////////////////////////////////////////
  // get the number of options like in: https://stackoverflow.com/questions/34134074/c-size-of-two-dimensional-array
  int row_num = sizeof(values) / sizeof(values[0]);
  int option_num = sizeof(values) / row_num / sizeof(long);
  //Serial.print("option_num: ");
  //Serial.println(option_num);
  //////////////////////////////////////////////////////////
  if(currentScreen==2){option_num=2;} // manual mode has 2 options only: on/off 
  //////////////////////////////////////////////////////////
  if (key == 0) {
    if (parameters[currentScreen] == (option_num - 1)) {
      parameters[currentScreen] = 0;
    }
    else {
      parameters[currentScreen]++;
    }
  }
  else if (key == 1) {
    if (parameters[currentScreen] == 0) {
      parameters[currentScreen] = (option_num - 1);
    }
    else {
      parameters[currentScreen]--;
    }
  }
  key_press_flag=1;
  parameter_screen=currentScreen;
  if(currentScreen!=2){
    parameter_value=values[currentScreen][parameters[currentScreen]];
  }
  else{
    parameter_value=parameters[currentScreen];
    }
//Serial.print("screen numer: ");
//Serial.print(currentScreen);
//Serial.print(", parameter_value: ");
//Serial.println(parameter_value);
}

//print to LCD function
void printScreen() { 
  unsigned long value=values[currentScreen][parameters[currentScreen]];
  //unsigned long value=parameter_value;
  float lcd_print_value;
  
  //Serial.println(value);
  //Serial.println(parameters[currentScreen]);

  if(currentScreen!=2){
      if(value<60){
         screens[currentScreen][1]="seconds";
           lcd_print_value=value;
      }
      else if(value>=60 && value<3600 ){
          screens[currentScreen][1]="minutes";
          lcd_print_value=value/60;
      }
      else if(value>=3600 && value<((long) 24*3600)){
        screens[currentScreen][1]="hours";
        lcd_print_value=value/(3600);
      }
      else{
        screens[currentScreen][1]="days";
        lcd_print_value=value/((long) 24*3600);
      }
    
      lcd.clear();
      lcd.print(screens[currentScreen][0]);
      lcd.setCursor(0, 1);
      //lcd.print(parameters[currentScreen]);
      lcd.print((int)lcd_print_value);
      lcd.print(" ");
      lcd.print(screens[currentScreen][1]);
  }
  else{
        lcd.clear();
        lcd.print(screens[currentScreen][0]);
        lcd.setCursor(0, 1);
        lcd.print(screens[currentScreen][1]);
      
  }
  lcd.setCursor(15, 0);
  lcd.write(byte(1)); //write my special rain char 
  lcd.setCursor(15, 1);
  lcd.write(byte(0)); //write my special pant char :))

}
void printToSerial() {
  Serial.print("screen ");
  Serial.print(currentScreen);
  Serial.print(", user Chose: ");
  Serial.print((float) rest_time/3600,5); //rest_time is in seconds so we divide by 3600 to get hours
  Serial.print(" hours interval at ");    //
  Serial.print((float) OnTime/1000,5);    //OnTime is in milisecs so divide by 1000 to get secs
  Serial.println(" seconds on-time");
}