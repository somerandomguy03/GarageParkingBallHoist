/*
GARAGE BALL HOIST THING SOFTWARE V1.0 - Good Luck everybody!
By:  https://www.thingiverse.com/chistompso/designs
Written:  January 2022

Future updates?  Well yes, there will be those.
I will add wireless capability, but one thing at a time. Also this code is an amateur-hour hot 
mess of disorgnization and inefficiency, so I plan to clean that up as well.

Let's just call this thing, "the device", OK?

This sketch controls up and down movement of tennis balls hung in a garage
for parking cars in just the right spot. This allows the balls to be retracted
out of the way when there is no car present in the garage/parking spot.  It also keeps
the little ones from practicing their tennis / baseball skills on it while there
is no car.  I decided to do this after having walked into said tennis ball while
car was not present.  

1.  When powered on, the user will be prompted to press the [Set] button to start the 
Hoping a homing operation for each ball, pulling them up until the limit switch is hit. 
Then, they lower a little to back off of the switch. NOTE:  If you only have a single garage
door or car position, you can stop the homing operation of the empty position by pressing
the limit switch with your finger.

2.  After the homing operation is performed, the device will check the EEPROM on the Arduino
to determine if it has already been set (saved the duration that each motor should run). If
there is no indication of a previous run in EEPROM the buttons will promp the user to press the
[Set] button.  Note:  This is button-specific.  

3. The setting operation is as follows:
3a.  Press [Set]
3b.  Tap the button for the position that you want to set
3c.  The selected position will run through the homing operation to return it to its home point.
3d.  The user is prompted to hold the button until the ball reaches the desired height. Note:  It is OK 
if the ball overshoots a little, as the code was written to account for a slight delay in human respone
time.
3e.  Once the desired height is achieved, and the user has released the button, they will be promped
to tap [Set] to complete the operation, or the opposite position's button to cancel the operation.
3f.  Once [Set] has been pressed, the user will be prompted to Tap the position they just set to 
start complete the operation, which will result in the ball returning to the home position.

4.  Normal operation of the tennis balls is as prompted on the buttons:  Tap for Up or Tap for Down


POWER - While the device will run off of a 9V battery, it is recommended that an AC power adapter be used
to ensure a consistent power supply.

Portions of this code, particuarly for the SSD1306 displays and TCA9548A multiplexer were derived
from the DroneBot Workshop:  https://dronebotworkshop.com/multiple-i2c-bus/

Side note:  
Could this have been done with stepper motors?  (i.e., not timed) Yes!  Howevever, we're 
talking about parking a car in a garage, we don't need pinpoint accuracy (at least you shouldn't)! 
If you are really picky, you could get hooks with a longer threaded shaft that you can use to fine
tune the ball's position when down.  The professional developers are already rolling their eyes 
and sighing and we haven't even gotten to the actual code yet.  To be fair, I thought about stepper 
motors after I had already made a bit of progress on this project.  And while I'm aware of the 
sunken cost fallacy and blah blah blah, I am human and decided,I can do some mental gymnastics to 
justify my lesser design solution.  So there!

Most of all, I hope you have fun with this project!  I wouldn't publish it if it didn't work! Also,
as stated earlier, updates are planned.

*/

#include <Wire.h> // Wire library for I2C
#include <Adafruit_GFX.h> //Graphics library
#include <Adafruit_SSD1306.h> //OLED library
#include <EEPROM.h> // We'll use this to store the set run times for each motor if device loses power.

Adafruit_SSD1306 Display1(-1);
Adafruit_SSD1306 Display2(-1);

int Hoist1Pos = 2; // 0 = Post Set Pause, 1 = Up, 2 = Down, 3 = In Motion, 4 Set Mode Gen, 5 Set Mode Down, 6 Set Mode Up
int Hoist2Pos = 2; // 0 = Post Set Pause, 1 = Up, 2 = Down, 3 = In Motion, 4 Set Mode Gen, 5 Set Mode Down, 6 Set Mode Up
int SetMode = 0; // 0 = SetMode is off (Operation Mode is on), 1 = SetMode is on (Operation Mode is off)
int SetPos = 0; // 0 = No selection, 1 = Set Hoist 1, 2 = Set Hoist 2
int UpToHome; // Variable used to call myHomingProcedure
int JustSet = 0; // Used to prevent looping back into Set mode after user hits the Set button to save a Set operation.  i.e., we just ran that Set operation, so don't assume I want to do it agian right away.

long MotorStart; // stores last time motor started running
long MotorStop; // stores last time motor stopped running
long Disp1Time; // store start time of Display 1
long Disp2Time; // stores start time of Display 2
long Motor1RunLimit = 0; //Default maximum down runtime for motor 1 is 0 seconds.  We will store this at EEPROM Address 10 and 11. When no value has been set, Address 30 will not contain value 100 (probably). 
long Motor2RunLimit = 0; //Default maximum down runtime for motor 2 is 0 seconds.  We will store this at EEPROM Address 20 and 21. When no value has been set, Address 31 will not contain value 100 (probably).
//My testing showed unset EEPROm memory addresses return -1, but who knows what you have. I dont know that you even have a fresh Arduino.  Do you know where your Arduino has been

const int Hoist1Button = 10; // Pin 10 for button to operate Hoist 1
const int Hoist2Button = 11; // Pin 11 for button to operate Hoist 2
const int SetButton = 2; // Pin 2 for button to put Hoist into Set Mode

const int M1Con1 = 5; //PWM Pin 5 for Motor 1, (Enable 1,2 on L293D Chip)
const int M1Con2 = 3; //Pin 3 for Motor 1 (Input 2 on L239D Chip)
const int M1Con3 = 4; //Pin 4 for Motor 1 (Input 1 on L239D Chip)
const int M2Con1 = 9; //PWM Pin 9 for Motor 2, (Enable 3,4 on L239D Chip)
const int M2Con2 = 6; //Pin 6 for Motor 2 (Input 3 on L239D Chip)
const int M2Con3 = 7; //Pin 7 for Motor 2 (Input 4 on L239D Chip)

const int LimitSwitch1 = 8; // Control pin 8 for limit switch 1
const int LimitSwitch2 = 12; // Control pin 12 for limit switch 2

//Pin A4 = SDA for Muliplexer.  Doesn't need to be declared.
//Pin A5 = SCL for Muliplexer.  Doesn't need to be declared.

void TCA9548A(uint8_t bus){
  Wire.beginTransmission(0x70);  // TCA9548A address is 0x70 
  Wire.write(1 << bus);          // send byte to select bus
  Wire.endTransmission();    
}

void displayInfo(int DisplayNum, String ShowInfo){
  if(DisplayNum == 1){
    TCA9548A(1);
    // Clear the display
    Display1.clearDisplay();
    //Set the color - always use white despite actual display color
    Display1.setTextColor(WHITE);
    //Set the font size
    Display1.setTextSize(2);
    //Set the cursor coordinates
    Display1.setCursor(0,10);
    Display1.print(ShowInfo);
    Display1.display();
  }
  if(DisplayNum == 2){
    TCA9548A(2);
    // Clear the display
    Display2.clearDisplay();
    //Set the color - always use white despite actual display color
    Display2.setTextColor(WHITE);
    //Set the font size
    Display2.setTextSize(2);
    //Set the cursor coordinates
    Display2.setCursor(0,10);
    Display2.print(ShowInfo);
    Display2.display();
  }  
}

void writeEEPROM(int MemAddress, int MotorRunTime)
{ 
  EEPROM.update(MemAddress, MotorRunTime >> 8);
  EEPROM.update(MemAddress + 1, MotorRunTime & 0xFF);
}

int readEEPROM(int MemAddress)
{
  return (EEPROM.read(MemAddress) << 8) + EEPROM.read(MemAddress + 1);
}

void setup() {
  Wire.begin(); //Start Wire library for I2C
  
  // Set multiplexer to channel 1 and initialize OLED-0 with I2C addr 0x3C
  TCA9548A(1);
  Display1.begin(SSD1306_SWITCHCAPVCC, 0x3C); //0x3C is default for 128x32 display
  // initialize OLED-1 with I2C addr 0x3C
  TCA9548A(2);
  Display2.begin(SSD1306_SWITCHCAPVCC, 0x3C); //0x3C is default for 128x32 display
  
  pinMode(M1Con1, OUTPUT);  
  pinMode(M1Con2, OUTPUT);
  pinMode(M1Con3, OUTPUT);
  
  pinMode(M2Con1, OUTPUT);  
  pinMode(M2Con2, OUTPUT);
  pinMode(M2Con3, OUTPUT);
  
  pinMode(LimitSwitch1, INPUT_PULLUP);
  pinMode(LimitSwitch2, INPUT_PULLUP);
  
  pinMode(Hoist1Button, INPUT_PULLUP);
  pinMode(Hoist2Button, INPUT_PULLUP);
  pinMode(SetButton, INPUT_PULLUP);
  
  displayInfo(1,"Hello!");
  displayInfo(2,"     :)");
  
  delay(3000);
  
  displayInfo(1,"Version:");
  displayInfo(2,"20220101");
  
  delay(5000);
  
  displayInfo(1,"Set4Home->");
  displayInfo(2,"<-Set4Home");
  
  //Since we just powered on, perform homing op.
  while((digitalRead(SetButton) == HIGH )){
    //do nothing
  }
 
  UpToHome = myHomingProcedure(M1Con1,M1Con2,M1Con3,LimitSwitch1,1); 
  UpToHome = myHomingProcedure(M2Con1,M2Con2,M2Con3,LimitSwitch2,2);  
  
  //Determine if Motor Run Limits have already been set.
  //EEPROM Address 30 will = 100 if Motor 1 has a set run time already
  /*
  if (readEEPROM(30) != 100) { 
    displayInfo(1,"SetMeA->");
  }else{
    Motor1RunLimit = (readEEPROM(10));
    displayInfo(1,"Tap4Down");
  }
  //EEPROM Address 31 will = 100 if Motor 2 has a set run time already
  if (readEEPROM(31) != 100) { 
      displayInfo(2,"<-SetMeA");  
  }else{
    Motor2RunLimit = (readEEPROM(20)); 
    displayInfo(2,"<-Tap4Down");  
  }
  */
}

void loop() {

  //NORMAL OPERATION

  if((digitalRead(SetButton) == HIGH ) && (SetMode == 0)){ 
    JustSet = 0; // If setting operation just occurred, running a normal operation will now clear the "Just Set" condition, so setting mode can be used again.
   //Hoist 1 Normal Operation
    if (digitalRead(Hoist1Button) == LOW && Hoist1Pos == 1 && Motor1RunLimit != 0 && SetMode ==0){ //Button pushed, and hoist is up, user has set down position
      displayInfo(1,"GoingDown!");
      analogWrite(M1Con1, 153); //Down at half speed. (255 is full speed)
      digitalWrite(M1Con2,LOW); //Down
      digitalWrite(M1Con3,HIGH); //Down    
      delay(Motor1RunLimit); // Change this delay value to change how long motor runs hoist down.
      //Quick pulse of upward movement to ensure momentum of ball doesn't keep hoist moving, especially if no car present.
      analogWrite(M1Con1, 255); //RUN FULL SPEED  
      digitalWrite(M1Con2, HIGH); //Up
      digitalWrite(M1Con3, LOW); //Up
      delay(100);
      analogWrite(M1Con1, 0); //Stop motor.
      Hoist1Pos = 2; // Save position of hoist as down.
      displayInfo(1,"Tap4Up");
    } else if (digitalRead(Hoist1Button) == LOW && Hoist1Pos == 2 && SetMode == 0 && Motor1RunLimit != 0) { //Button pushed and hoist is down 
      displayInfo(1,"UpUp&Away!");
      UpToHome = myHomingProcedure(M1Con1,M1Con2,M1Con3,LimitSwitch1,1); 
      displayInfo(1, "Tap4Down");        
    } else if (digitalRead(Hoist1Button) == LOW && Hoist1Pos == 2 && SetMode == 0 && Motor1RunLimit == 0){
      displayInfo(1,"SetMe->");        
    }
    
    //Hoist 2 Normal Operation 
    if (digitalRead(Hoist2Button) == LOW && Hoist2Pos == 1 && Motor2RunLimit != 0 && SetMode ==0){ //Button pushed, and hoist is up, user has set down position
      displayInfo(2,"GoingDown!");
      analogWrite(M2Con1, 153); //Down at half speed. (255 is full speed)
      digitalWrite(M2Con2,HIGH); //Down
      digitalWrite(M2Con3,LOW); //Down    
      delay(Motor2RunLimit); // Change this delay value to change how long motor runs hoist down.
      //Quick pulse of upward movement to ensure momentum of ball doesn't keep hoist moving, especially if no car present.
      analogWrite(M2Con1, 255); //RUN FULL SPEED  
      digitalWrite(M2Con2, LOW); //Up
      digitalWrite(M2Con3, HIGH); //Up
      delay(100);
      analogWrite(M2Con1, 0); //Stop motor.
      Hoist2Pos = 2; // Save position of hoist as down.
      displayInfo(2, "Tap4Up");
    } else if (digitalRead(Hoist2Button) == LOW && Hoist2Pos == 2 && SetMode == 0 && Motor2RunLimit != 0) { //Button pushed and hoist is down 
      displayInfo(2,"UpUp&Away!");
      UpToHome = myHomingProcedure(M2Con1,M2Con2,M2Con3,LimitSwitch2,2); 
      displayInfo(2,"Tap4Down");         
    } else if (digitalRead(Hoist2Button) == LOW && Hoist2Pos == 2 && SetMode == 0 && Motor2RunLimit == 0){
      displayInfo(2,"<-SetMe");  
    }
  }

  //TOGGLE SET MODE ON AND OFF
  //SET MODE ON
  if ((Hoist1Pos != 3) && (Hoist2Pos!=3) && (digitalRead(SetButton) == LOW) && (JustSet == 0)){  
    if (SetMode == 0) {
      SetMode = 1;
      displayInfo(1,"Tap2Select");
      displayInfo(2,"Tap2Select");
  //SET MODE OFF, TO SAVE OR NOT TO SAVE
    }else if (SetMode == 1) {
      if ((SetPos == 1) && (MotorStop - MotorStart > 1000)){ //Record new Motor1RunLimit of set time of travel is greater than 2 seconds.
        Motor1RunLimit = (MotorStop - MotorStart) - 100; //Shave a little time off to account for human reaction time in releasing the button.
        writeEEPROM(10, Motor1RunLimit);
        writeEEPROM(30, 100);
        JustSet = 1; //We just ran the Set operation, so we use this to prevent the Set button from getting stuck in a loop.  The first normal opreation will clear it.
        displayInfo(1,"Tap2Compl");
        if (Motor2RunLimit !=0){
          if (Hoist2Pos==1) {
            displayInfo(2,"Tap4Down");
          }else if (Hoist2Pos == 2) {
            displayInfo(2,"Tap4Up");
          }
        } else {
          displayInfo(2,"<-SetMe");
        }
      } else if ((SetPos == 2) && (MotorStop - MotorStart > 1000)){ //Record new Motor2RunLimit of set time of travel is greater than 2 seconds.
        Motor2RunLimit = (MotorStop - MotorStart) - 100;  //Shave a little time off to account for human reaction time in releasing the button.
        writeEEPROM(20, Motor2RunLimit);
        writeEEPROM(40, 100);
        JustSet = 1; //We just ran the Set operation, so we use this to prevent the Set button from getting stuck in a loop.  The first normal opreation will clear it.
        displayInfo(2,"Tap2Compl");
        if (Motor1RunLimit !=0){
          if (Hoist1Pos==1) {
            displayInfo(1,"Tap4Down");
          } else if (Hoist1Pos == 2) {
            displayInfo(1,"Tap4Up");
          }
        } else {
          displayInfo(1,"SetMe->");
        }
      }
      SetMode = 0;
      SetPos = 0;
    }
  }
    
  //CANCEL SET MODE WITH HOIST BUTTON  
  if ((SetMode == 1) && (digitalRead(Hoist1Button) == LOW) && (Hoist2Pos == 2) && (SetPos == 2)) { //Cancel setting for Hoist 2 by pressing Hoist 1 button
    Disp1Time = millis();
    if (millis() - Disp1Time < 3000){
      displayInfo(1,"SetCancelled");
      displayInfo(2,"SetCancelled");
    }else{
    SetMode = 0;
    SetPos = 0;     
    }
  } else if ((SetMode == 1) && (digitalRead(Hoist2Button) == LOW) && (Hoist1Pos == 2) && (SetPos == 1)) { //Cancel setting for Hoist 1 by pressing Hoist 2 button
    Disp1Time = millis();
    if (millis() - Disp1Time < 3000){
      displayInfo(1,"SetCancelled");
      displayInfo(2,"SetCancelled");
      delay(3000);
    }else{
    SetMode = 0;
    SetPos = 0;
    }
  }  
  
  //UPDATE DISPLAYS FOR SET MODE 
  if ((SetMode == 1) && (digitalRead(Hoist1Button) == LOW) && (Hoist1Pos = 1) && (SetPos == 0)) { //Hoist 1 has been selected for setting
    SetPos = 1;    
    UpToHome = myHomingProcedure(M1Con1,M1Con2,M1Con3,LimitSwitch1,1);
    displayInfo(1,"Hold4Dwn");
    displayInfo(2,"Set4Sv/Me4No");
  }
  if ((SetMode == 1) && (digitalRead(Hoist2Button) == LOW) && (Hoist2Pos == 1) && (SetPos == 0)) {
    SetPos = 2;
    UpToHome = myHomingProcedure(M2Con1,M2Con2,M2Con3,LimitSwitch2,2);
    displayInfo(1,"Set4Sv.Me4No");  
    displayInfo(2,"Hold4Dwn");
  }
  
  //EXECUTE SET
  if (digitalRead(Hoist1Button == LOW) && (SetMode == 1) && (digitalRead(Hoist1Button) == LOW) && (Hoist1Pos = 1) && SetPos == 1) {
    MotorStart = millis();
    while (digitalRead(Hoist1Button) == LOW) { //Lower hoist as long as user is pressing SetButton and Hoist1Button
      displayInfo(1,"LetGo2Stop");
      analogWrite(M1Con1, 153); //Down at half speed.
      digitalWrite(M1Con2,LOW); //Down
      digitalWrite(M1Con3,HIGH); //Down    
    }
    analogWrite(M1Con1, 0); //Stop motor.
    MotorStop = millis(); //Record time that motor stopped.
    Hoist1Pos = 2; //Hoist is now down
  }

  if (digitalRead(Hoist2Button == LOW) && (SetMode == 1) && (digitalRead(Hoist2Button) == LOW) && (Hoist2Pos = 1) && SetPos == 2){
    MotorStart = millis();
    while (digitalRead(Hoist2Button) == LOW) { //Lower hoist as long as user is pressing SetButton and Hoist1Button
      displayInfo(2,"LetGo2Stop");
      analogWrite(M2Con1, 153); //Down at half speed.
      digitalWrite(M2Con2,HIGH); //Down
      digitalWrite(M2Con3,LOW); //Down    
    }
    analogWrite(M2Con1, 0); //Stop motor.
    MotorStop = millis() ; //Record time that motor stopped.
    Hoist2Pos = 2; //Hoist is now down
  }
}

int myHomingProcedure (int MPin1, int MPin2, int MPin3, int LSwitch, int MotorNum) {
  //Perform the homing procedure
  displayInfo(MotorNum,"Go'n Home!"); //Display that we are homing on display 1    
  while (digitalRead(LSwitch) == LOW) { /*retract until switch is pressed
    Switch Comm is to GND.  Normally Closed (NC)
    When switch is pressed, resistance becomes high.
    If wire break/disconnect occurs, resistance is also high, stopping motors (fail safe).
    */
  if (MotorNum ==1) {
    analogWrite(MPin1, 255); //RUN FULL SPEED  
    digitalWrite(MPin2, HIGH); //Up
    digitalWrite(MPin3, LOW); //Up
   } else {
    analogWrite(MPin1, 255); //RUN FULL SPEED  
    digitalWrite(MPin2, LOW); //Up
    digitalWrite(MPin3, HIGH); //Up
   }
  }
  analogWrite(MPin1, 0); //Stop motor.
  while (digitalRead(LSwitch) == HIGH) { /*do until switch is not pressed
    This backs off of the limit switch to put the hoist in its home position
    */
    if (MotorNum ==1){
      analogWrite(MPin1, 255); //RUN FULL SPEED
      digitalWrite(MPin2,LOW); //Down
      digitalWrite(MPin3,HIGH); //Down
    }else{
      analogWrite(MPin1, 255); //RUN FULL SPEED
      digitalWrite(MPin2,HIGH); //Down
      digitalWrite(MPin3,LOW); //Down
    }
  }
  analogWrite(MPin1, 0); //Stop motor.
  if (MotorNum == 1) {
    Hoist1Pos = 1; // We know the hoist is up after performing homing procedure.
  } else if (MotorNum == 2) { 
    Hoist2Pos = 1;
  }
  displayInfo(MotorNum,"Homed!");
  //Determine if Motor Run Limits have already been set.
  //EEPROM Address 30 will = 100 if Motor 1 has a set run time already
  if (Hoist1Pos ==1) {
    if (readEEPROM(30) != 100) { 
      displayInfo(1,"SetMe->");
    }else{
      Motor1RunLimit = (readEEPROM(10));
      displayInfo(1,"Tap4Down");
    }
  }
  //EEPROM Address 31 will = 100 if Motor 2 has a set run time already
  if (Hoist2Pos ==1) {  
    if (readEEPROM(40) != 100) { 
        displayInfo(2,"<-SetMe");  
    }else{
      Motor2RunLimit = (readEEPROM(20)); 
      displayInfo(2,"Tap4Down");  
    }
  }
}


