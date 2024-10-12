// includes relevant libraries
#include <dht.h>
#include <LiquidCrystal_I2C.h>
#include <wire.h>
#include <arduino-timer.h>
#include <SoftwareSerial.h>


dht DHT; //Creates a dht object called DHT
auto greenLEDTimer = timer_create_default(); // creates timer for green standby light
auto redLEDTimer = timer_create_default(); // Creates timer for red error light
LiquidCrystal_I2C lcd(0x27, 20, 4); // Sets up LCD display with 4 rows and 20 columns

// Sets values for checks and values
int waterVal = 0;
int temperatureVal = 0;
int humidityVal = 0;
int DHTCheck = 0;

// Sets pins
const int waterPin = A5;
const int buzzerPin = 13;
const int tiltPin = 12;
const int DHTPin = 2;
const int redLEDPin = 5;
const int greenLEDPin = 6;

// Sets minimum and maximum default values
int tempMax = 70;
int tempMin = 0;
int waterMax = 100;
int waterMin = 000;
int humidityMax = 100;
int humidityMin = 00;

// Creates variables for HC05 and its checks
String HC05Data = ""; // Sets empty string for HC05 data input
int valChange = 0; // Lets you know which value the user is changing
bool isNum =  true; // Lets you know if what they entered is a number


void setup() {
  // Sets up serial and default outputs
  Serial.begin(9600);
  pinMode (redLEDPin, OUTPUT);
  pinMode (greenLEDPin, OUTPUT);
  pinMode (buzzerPin, OUTPUT);
  pinMode (tiltPin, INPUT_PULLUP);

  // Initialise LCD
  lcd.init();

  // Check to change address of LCD if it's wrong (Mostly redundant)
  if (!i2CAddrTest(0x27)) {
    lcd = LiquidCrystal_I2C(0x3F, 20, 4);
  }

  // Sets up default pin states
  digitalWrite(redLEDPin, LOW);
  digitalWrite(greenLEDPin, LOW);
  digitalWrite(buzzerPin, LOW);

  // Sets timers
  greenLEDTimer.every(10000, powerLED); // Every 10 seconds call powerLED function whilst ticking
  redLEDTimer.every(1000, errorLED); // Every second call errLED function whilst ticking

  // Prints first-time command list prompt to terminal
  Serial.println("For a list of commands please type 'Help'.");    
}

void loop() {
  // Take readings from the various pins
  waterVal = analogRead(waterPin);
  DHTCheck = DHT.read11(DHTPin);
  temperatureVal = DHT.temperature;
  humidityVal = DHT.humidity;

  // Ticks the green LED timer to flash standby
  greenLEDTimer.tick();

  // First checks if tiltpin is disconnected, as this is the most important error. If so then enters the tiltErr function
  if(digitalRead(tiltPin) == HIGH){
    tiltErr();
  }

  // Then checks values against ranges to see if any out of range. If so then called the outOfRangeErr() function
  if (((temperatureVal > tempMax) || (temperatureVal < tempMin)) || ((waterVal > waterMax) || (waterVal < waterMin)) || ((humidityVal > humidityMax) || (humidityVal < humidityMin))){
    outOfRangeErr();
  }

  // Checks for HC05 input without sticking in wait mode
  if(Serial.available() > 0){
    // Reads the string from the serial and removes any tabs, spaces, or enters
    HC05Data = Serial.readString();
    HC05Data.trim();
    HC05Check();
  }
}

// Function called by greenLEDTimer to flash green led
bool powerLED(void *) {
  digitalWrite(greenLEDPin, HIGH); // toggle the LED
  delay(100);
  digitalWrite(greenLEDPin, LOW);
  return true; // keep timer active? true
}

// Function called by redLEDTimer to flash red led
bool errorLED(void *) {
  digitalWrite(redLEDPin, HIGH); // toggle the LED
  delay(500);
  digitalWrite(redLEDPin, LOW);
  return true; // keep timer active? true
}

// Function to alert the user if the box has tilted
void tiltErr() {
  // sets the outputs to default expected
  lcd.noBacklight();
  digitalWrite(greenLEDPin, LOW); // just in case was on when loop started
  digitalWrite(buzzerPin, HIGH);
  Serial.print("The Plant Monitor has fallen over!");

  while ((digitalRead(tiltPin) == HIGH)){
    redLEDTimer.tick(); // Ticks error timer to flash red light

    if(Serial.available() > 0){
      // Reads the string from the serial and removes any tabs, spaces, or enters
      HC05Data = Serial.readString();
      HC05Data.trim();
      HC05Check();
    }
  }

  // once no longer tilted will turn outputs off and exit
  digitalWrite(buzzerPin, LOW);
  digitalWrite(redLEDPin, LOW);
  Serial.print("The Plant Monitor is upright.");
  return;
}

// Function to alert the user if a value has gone out of range
void outOfRangeErr() {
  // First sends a message dependent on what has gone wrong, only does this once
  digitalWrite(greenLEDPin, LOW); 
  lcd.backlight();
  if (temperatureVal > tempMax){
    Serial.print("Temperature too high!");
  }
  else if (temperatureVal < tempMin){
    Serial.print("Temperature too low!");
  }
  if (waterVal > waterMax){
    Serial.print("Water level too high!");
  }
  else if (waterVal < waterMin){
    Serial.print("Water level too low!");
  }
  if (humidityVal > humidityMax){
    Serial.print("Humidity too high!");
  }
  else if (humidityVal < humidityMin){
    Serial.print("Humidity too low!");
  }

  // While an issue remains will stick in this loop
  while (((temperatureVal > tempMax) || (temperatureVal < tempMin)) || ((waterVal > waterMax) || (waterVal < waterMin)) || ((humidityVal > humidityMax) || (humidityVal < humidityMin))){
    // Checks the readings
    DHTCheck = DHT.read11(DHTPin);
    temperatureVal = DHT.temperature;
    humidityVal = DHT.humidity;
    waterVal = analogRead(waterPin);
  
    // Ticks the timer for the error LED
    redLEDTimer.tick();

    // tiltErr is more important so will switch to that over staying in the out of range state if also falls over
    if(digitalRead(tiltPin) == HIGH){
      tiltErr();
      lcd.backlight();
    }

    // Clears the LCD and moves the cursor back to the start
    lcd.clear();
    lcd.setCursor(0,0);

    // Prints details to 
    lcd.print("Temp:");
    lcd.print(temperatureVal);
    if (temperatureVal > tempMax) lcd.print(" Too high!");
    else if (temperatureVal < tempMin) lcd.print(" Too low!");
    else lcd.print(" Nominal");

    lcd.setCursor(0,1);
    lcd.print("Water:");
    lcd.print(waterVal);
    if (waterVal > waterMax) lcd.print(" Too high!");
    else if (waterVal < waterMin) lcd.print(" Too low!");
    else lcd.print(" Nominal");

    lcd.setCursor(0,2);
    lcd.print("Humidity:");
    lcd.print(humidityVal);
    if (humidityVal > humidityMax) lcd.print(" Too high!");
    else if (humidityVal < humidityMin) lcd.print(" Too low!");
    else lcd.print(" Nominal");

    if(Serial.available() > 0){
    // Reads the string from the serial and removes any tabs, spaces, or enters
    HC05Data = Serial.readString();
    HC05Data.trim();
    HC05Check();
    }
  }

  // Once no errors writes outputs and turns LCD off for power conservation
  lcd.noBacklight();
  Serial.print("All readings nominal.");
  digitalWrite(redLEDPin, LOW);
  return;
}

void HC05Check() {
      //Does checks to see if the user input something which we recognise and actions
  if (HC05Data == "tempMax"){
    valChange = 1;
    Serial.println("Please enter a number to change the maximum temperature.");
    HC05Data = ""; // Resets the HC05 string after each command
  }
  else if (HC05Data == "tempMin"){
    valChange = 2;
    Serial.println("Please enter a number to change the minimum temperature.");
    HC05Data = "";
  }
  else if (HC05Data == "waterMax"){
    valChange = 3;
    Serial.println("Please enter a number to change the maximum water level.");
    HC05Data = "";
  }
  else if (HC05Data == "waterMin"){
    valChange = 4;
    Serial.println("Please enter a number to change the minimum water level.");
    HC05Data = "";
  }
  else if (HC05Data == "humidityMax"){
    valChange = 5;
    Serial.println("Please enter a number to change the maximum humidity.");
    HC05Data = "";
  }
  else if (HC05Data == "humidityMin"){
    valChange = 6;
    Serial.println("Please enter a number to change the minimum temperature.");
    HC05Data = "";
  }
  else if (HC05Data == "Readout"){
    Serial.print("Temperature = ");
    Serial.print(DHT.temperature);
    Serial.print(" Water Level = ");
    Serial.print(analogRead(waterPin));
    Serial.print(" Humidity = ");
    Serial.print(DHT.humidity);
    Serial.print("\n");
    HC05Data = "";
  }
  else if (HC05Data == "Ranges"){
    Serial.println("The range for temperature is from ");
    Serial.print(tempMin);
    Serial.print(" to ");
    Serial.print(tempMax);
    Serial.println("The range for water level is from ");
    Serial.print(waterMin);
    Serial.print(" to ");
    Serial.print(waterMax);
    Serial.println("The range for humidity level is from ");
    Serial.print(humidityMin);
    Serial.print(" to ");
    Serial.print(humidityMax);
    HC05Data = "";
  }
  else if (HC05Data == "Buzzer"){
    digitalWrite(buzzerPin, LOW);
    Serial.println("The buzzer has been temporarily disabled.");
    HC05Data = "";
  }
  else if (HC05Data == "Help"){
    Serial.println("The list of commands is:");
    Serial.println("'tempMax' allows you to set the maximum temperature before alert.");
    Serial.println("'tempMin' allows you to set the minimum temperature before alert.");
    Serial.println("'waterMax' allows you to set the maximum water level before alert.");
    Serial.println("'waterMin' allows you to set the minimum water level before alert.");
    Serial.println("'humidityMax' allows you to set the maximum humidity level before alert.");
    Serial.println("'humidityMin' allows you to set the minimum humidity level before alert.");
    Serial.println("'Readout' displays the current readout of the sensors.");
    Serial.println("'Ranges' displays the current nominal ranges of the sensors.");
    Serial.println("'Buzzer' toggles turns the buzzer off for that error (temporarily).");
    HC05Data = "";
  }
  else{
    isNum =  true; // First expects number if not one of the previous strings

    // Enters for loop to confirm if the code is made up of all digits, if so proceeds but if not then changes isNum to false and exits loop
    for (int i = 0; i<(HC05Data.length()); i++){
      if (isDigit(HC05Data[i])){
        continue;
      }
      else {
        isNum = false;
        HC05Data = "";
        break;
      }
    }
    // If it is a number then it will either change the value corresponding to the valChange integer or tell you there is no value selected to change
    if (isNum == true){
      if(valChange ==1){
        tempMax = HC05Data.toInt();
        Serial.println("The new value of tempMax is ");
        Serial.print(HC05Data.toInt());
        HC05Data = "";
      }
      else if (valChange == 2){
        tempMin = HC05Data.toInt();
        Serial.println("The new value of tempMin is ");
        Serial.print(HC05Data.toInt());
        HC05Data = "";
      }
      else if (valChange == 3){
        waterMax = HC05Data.toInt();
        Serial.println("The new value of waterMax is ");
        Serial.print(HC05Data.toInt());
        HC05Data = "";
      }
      else if (valChange == 4){
        waterMin = HC05Data.toInt();
        Serial.println("The new value of waterMin is ");
        Serial.print(HC05Data.toInt());
        HC05Data = "";
      }
      else if (valChange == 5){
        humidityMax = HC05Data.toInt();
        Serial.println("The new value of humidityMax is ");
        Serial.print(HC05Data.toInt());
        HC05Data = "";
      }
      else if (valChange == 6){
        humidityMin = HC05Data.toInt();
        Serial.println("The new value of humidityMin is ");
        Serial.print(HC05Data.toInt());
        HC05Data = "";
      }
      else{
        Serial.println("Please first enter a value to change.");
      }
    }
    else{ // If not a number or command will return unknown command
      Serial.println("Unknown Command.");
      HC05Data = "";
    }
  }
  return;
}  

// Function required for LCD address change (Mostly redundant)
bool i2CAddrTest(uint8_t addr) {
  Wire.begin();
  Wire.beginTransmission(addr);
  if (Wire.endTransmission() == 0) {
    return true;
  }
  return false;
}