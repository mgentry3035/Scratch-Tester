#include <Nextion.h>
#include <AccelStepper.h>
#include <Servo.h>
#include <ezButton.h>

//Limit switches
//Not used for this version
ezButton leftEn(40);
ezButton backEn(41);
ezButton forwardEn(38);
ezButton rightEn(39);

//Stepper motor setup
#define xStepPin 2
#define xDirPin 5
AccelStepper stepperX(AccelStepper::DRIVER, xStepPin, xDirPin);
#define yStepPin 3
#define yDirPin 6
AccelStepper stepperY(AccelStepper::DRIVER, yStepPin, yDirPin);
int speed = 10;
long xPosAdjust = 0;
long yPosAdjust = 0;

//X and Y steps per Mm
//Conversion needed for better precision
uint32_t xStepsPerMm = 8;
uint32_t yStepsPerMm = 40;
uint32_t scratchSpacing = 10; //distance between scratches in mm

//Variables to track stepper movements during test
long stepsToGo = 0;
int currPos;

bool acceptInput;  //determines when we're accepting input.


//Not used for this version
bool rightRunning = false;
bool leftRunning = false;
bool forwardRunning = false;
bool backRunning = false;

//Nextion interface setup
NexPage page0 = NexPage(0, 0, "mainPage");
NexPage page1 = NexPage(1, 0, "stopPage");

NexButton backButton = NexButton(0, 1, "backButton");
NexButton forwardButton = NexButton(0, 2, "forwardButton");
NexButton leftButton = NexButton(0, 3, "leftButton");
NexButton rightButton = NexButton(0, 4, "rightButton");

NexButton homeButton = NexButton(0, 10, "homeButton");
NexButton pinDownButton = NexButton(0, 5, "pinDownButton");

NexSlider scrLenSlider = NexSlider(0, 6, "scrLenSlider");

NexButton startButton = NexButton(0, 9, "startButton");
NexButton stopButton = NexButton(1, 1, "stopButton");


//Speed control for Test, unit steps per second
#define initTestSPD -960//initial speed on start push, <0 to move pin in correct direction
#define retTestSPD 1000//speed for pin return on start push
#define startXSPD 500//speed in X dir at the end of test

#define finalXDIST 80//steps for final movement in X DIR

NexTouch *nex_listen_list[] =
{
	&backButton,
	&forwardButton,
	&leftButton,
	&rightButton,
	&homeButton,
	&pinDownButton,
	&startButton,
	&stopButton,
	NULL
};

//Pin control servo setup
Servo pinControlServo;


uint32_t scratchLength = 50;

//global variables for Pin definitions.  Informational for hardware setup
uint32_t xStop = 9;
uint32_t yStop = 10;
uint32_t dropPin = 16;
const int enPin = 8;

void nextionWrite(String message) {
    Serial3.print(message);
    Serial3.print("\xFF\xFF\xFF"); // Nextion end command  Serial3.print("click stopButton, 1");
}

bool testLimitCheck(){
  if (backEn.isPressed()||forwardEn.isPressed()||leftEn.isPressed()||rightEn.isPressed()) return true;
  else return false;
}


void backPushCallback(void *ptr)
{
	Serial.println("Move backward");
  globalEnable(false);
  xPosAdjust = -5;//speed X stepper moves when back is pressed, <0 for direction
  if(backEn.getState() == HIGH){ 
    xPosAdjust = -5;
    backRunning = true;
  }
  else if(backEn.getState() == LOW) xPosAdjust = 0;
  if(backEn.isPressed()) xPosAdjust = 0;
}
void backReleaseCallback(void *ptr)
{
	Serial.print("Released");
  if(!digitalRead(xStop)) {
    stepperX.move(-xPosAdjust*2);
    while(stepperX.run());
  }
  xPosAdjust = 0;
  backRunning = false;
  globalEnable(true);
}

void limitLoop(){
  leftEn.loop();
  backEn.loop();
  rightEn.loop();
  forwardEn.loop();
}


void forwardPushCallback(void *ptr)
{
	Serial.println("Move forward");
  globalEnable(false);
  //xPosAdjust = speed;
  if(forwardEn.getState() == HIGH){
    xPosAdjust = 5;//speed X stepper moves when forward is pressed, >0 for direction
    forwardRunning = true;
  }
  else if(forwardEn.getState() == LOW) xPosAdjust = 0;
  if(forwardEn.isPressed()) xPosAdjust = 0;
}
void forwardReleaseCallback(void *ptr)
{
	Serial.println("Released");
  if(!digitalRead(xStop)){
    stepperX.move(-xPosAdjust*2);
    while(stepperX.run());
  }
  xPosAdjust = 0;
  forwardRunning = false;
  globalEnable(true);
}


void leftPushCallback(void *ptr)
{
	Serial.println("Move left");
  globalEnable(false);
  if(leftEn.getState() == HIGH){ 
    yPosAdjust = speed*2;//speed Y stepper moves when left is pressed, >0 for direction
    leftRunning = true;
  }
  else if(leftEn.getState() == LOW) yPosAdjust = 0;
  if(leftEn.isPressed()) yPosAdjust = 0; 
}
void leftReleaseCallback(void *ptr)
{
	Serial.println("Released");
  if(!digitalRead(yStop)){
    stepperY.move(-yPosAdjust*2);
    while(stepperY.run());
  }
  yPosAdjust = 0;
  leftRunning = false;
  globalEnable(true);
}


void rightPushCallback(void *ptr)
{
	Serial.println("Move right");
  globalEnable(false);
  if(rightEn.getState() == HIGH){
    yPosAdjust = -speed*2;//speed Y stepper moves when right is pressed, <0 for direction
    rightRunning = true;
  }
  else if(rightEn.getState() == LOW) yPosAdjust = 0;
  if(rightEn.isPressed()) yPosAdjust = 0;
}
void rightReleaseCallback(void *ptr)
{
	Serial.println("Released");
  if(!digitalRead(yStop)){  //if we've run into the stop while moving, back off
    stepperY.move(-yPosAdjust*2);
    while(stepperY.run());
  }
  yPosAdjust = 0;
  rightRunning = false;
  globalEnable(true);
}


//Not used
void homePushCallback(void *ptr)
{
	Serial.println("Sending Pin to Home");
  if(acceptInput){
		globalEnable(false);
    //send the send carriage to the home switches, then set the position to (0,0)
    xPosAdjust = speed;
    while(digitalRead(xStop)){  //if we've run into the stop while moving, back off
      stepperX.move(xPosAdjust);
      while(stepperX.run());
    }
    stepperX.move(-xPosAdjust*2);
    while(stepperX.run());
    stepperX.setCurrentPosition(0);

    //send the send carriage to the home switches, then set the position to (0,0)
    yPosAdjust = speed;
    while(digitalRead(yStop)){  //if we've run into the stop while moving, back off
      stepperY.move(yPosAdjust);
      while(stepperY.run());
    }
    stepperY.move(-yPosAdjust*2);
    while(stepperY.run());
    stepperY.setCurrentPosition(0);

    //search home switches at slow speed
    //set encoder values to (-10,-10). . . or something like that
    //move carriage to (0,0)
		globalEnable(true);
	}
}

//Functions for moving servo motor up and down
void pinDownPushCallback(void *ptr)
{
  Serial.println("Drop Pin");
	if(acceptInput){
	  pinControlServo.write(0); //move servo to 90 degrees
  }
}
void pinDownReleaseCallback(void *ptr)
{
	Serial.println("Lifting Pin");
  if(acceptInput){
		pinControlServo.write(180); //move to zero degrees
  }
}

//FUnction for starting test
bool testRunning = false;
void startCallback(void *ptr)
{
	Serial.println("Starting Test with scratch length: ");
  delay(10);
  scrLenSlider.getValue(&scratchLength);//receives scratch length from nextion device
  Serial.println(scratchLength);
  nextionWrite("page 1"); 
  
  testRunning = true;
  if(acceptInput){
		globalEnable(false);
  
		pinControlServo.write(0); //drop the pin
    delay(1200);//delay(ms) to let pin drop
    
    if (scratchLength == 0) {
      scratchLength = 50;
    }
    //run the scratch the specified length
    Serial.print("Scratch length set to: ");
    Serial.println(scratchLength);
    stepsToGo = scratchLength * yStepsPerMm;//Adjust yStepsPerMm if initial movement is inaccurate
    stepperY.setCurrentPosition(0);
    currPos = 0;
    while(testRunning && stepsToGo > 0){  //if we've run into the stop while moving, back off

     // limitLoop();

      Serial.print(stepsToGo);
      Serial.println(" steps to go.");

      //check back limit switch


      if (!digitalRead(yStop)) {
        Serial.println("Hit the yStop");
        testRunning = false;
        continue;
      }
      stepperY.moveTo(-stepsToGo);//keep <0 for correct direction
      stepperY.setSpeed(initTestSPD);//Set speed(sps) for initial pin movement
      while(currPos > (-stepsToGo)){//Move pin until it reaches desired location
        stepperY.runSpeed();
        currPos = stepperY.currentPosition();//Update current position until is reaches desired location
        nexLoop(nex_listen_list); //process any waiting input from Nextion display, which would be the Estop
      }
      stepsToGo = 0;
      
    }

      //exit early if test stopped
      if (!testRunning) {
        Serial.println("Test stopped during scratch movement");
        nextionWrite("page 0"); 
        testRunning = false;
        globalEnable(true);
        return;
      }

    pinControlServo.write(180); //pick up the pin
    delay(1200);//Delay to let pin be picked up
    
    //go back to the beginnning
    stepsToGo = scratchLength * yStepsPerMm;
    stepperY.setCurrentPosition(0);
    currPos = 0;
    //testRunning = true;
    while(testRunning && stepsToGo > 0){  //if we've run into the stop while moving, back off

      Serial.print(stepsToGo);
      Serial.println(" steps to go.");


      if (!digitalRead(yStop)) {
        testRunning = false;
        continue;
      }

      stepperY.moveTo(stepsToGo);//return pin to original location
      stepperY.setSpeed(retTestSPD);//Pin return movement speed(sps)

      while(currPos<stepsToGo){
        stepperY.runSpeed();
        currPos = stepperY.currentPosition();//update current position and check if it is where it needs to be
        nexLoop(nex_listen_list); //process any waiting input from Nextion display, which would be the Estopp
      }

      stepsToGo = 0;
    }


    if (!testRunning) {
      Serial.println("Test stopped during return movement");
      nextionWrite("page 0"); 
      testRunning = false;
      globalEnable(true);
      return;
    }

    //now move over <spacing> mm for the next test
    stepsToGo = finalXDIST;//moves board 5mm in X direction after scratch test is complete
    stepperX.setCurrentPosition(0);
    currPos = 0;
    while(testRunning && stepsToGo > 0){  //if we've run into the stop while moving, back off


      if (!digitalRead(xStop)) {
        testRunning = false;
        continue;
      }
      stepperX.moveTo(stepsToGo);
      stepperX.setSpeed(startXSPD);//sets speed for final board movement
      while(currPos < (stepsToGo)){
        stepperX.runSpeed();
        currPos = stepperX.currentPosition();
        nexLoop(nex_listen_list); //process any waiting input from Nextion display, which would be the Estop
      }
      stepsToGo = 0;
    }


            // If the test has stopped, exit early
    if (!testRunning) {
      Serial.println("Test stopped during spacing movement");
      nextionWrite("page 0"); 
      testRunning = false;
      globalEnable(true);
      return;
    }
    //test finished. Go to home screen
    nextionWrite("page 0"); 
    testRunning = false;
    globalEnable(true);
  }
}

void stopCallback(void *ptr)
{
  Serial.println("Emergency Stop Order Recieved");
   //pick up the pin
   pinControlServo.write(180); 
  //cancel all movement by telling it to go to where it is at (resets the values in the AccelStepper library)
  stepperX.stop();
  stepperY.stop();
  stepsToGo = 0;
  currPos = 0;
  testRunning = false;
  globalEnable(true);  //start accepting input
}

void globalEnable(boolean state) {
  acceptInput = state;
  if(state) {
    digitalWrite(enPin, HIGH);
  } else {
    digitalWrite(enPin, LOW);
  }
}

void setup() {
  //Configure enPin, which controls whether servos are powered.  Leave low until needed.
  pinMode(enPin,OUTPUT);

  //initialize the debug port and Nextion Display  
	Serial.begin(115200);
  Serial3.begin(9600);
	delay(500);
  Serial.println("Start initialization");

	// Configure the steppers
	globalEnable(false);
  stepperX.setMaxSpeed(3000);
	stepperY.setMaxSpeed(3000);
  stepperX.setSpeed(200);
  stepperY.setSpeed(200);
  stepperX.setAcceleration(4000);
  stepperY.setAcceleration(4000);
  pinMode(xStop, INPUT_PULLUP);
  pinMode(yStop, INPUT_PULLUP);

  leftEn.setDebounceTime(50);
  backEn.setDebounceTime(50);
  rightEn.setDebounceTime(50);
  forwardEn.setDebounceTime(50);
  //configure the servo
  pinControlServo.attach(dropPin);
  delay(10);
  
	//setup all the callbacks.  These are the functions that respond to Nextion Display actions
	backButton.attachPush(backPushCallback);
	backButton.attachPop(backReleaseCallback);
	forwardButton.attachPush(forwardPushCallback);
	forwardButton.attachPop(forwardReleaseCallback);

	leftButton.attachPush(leftPushCallback);
	leftButton.attachPop(leftReleaseCallback);
	rightButton.attachPush(rightPushCallback);
	rightButton.attachPop(rightReleaseCallback);

	pinDownButton.attachPush(pinDownPushCallback);
	pinDownButton.attachPop(pinDownReleaseCallback);
	homeButton.attachPush(homePushCallback);

	startButton.attachPop(startCallback);
	stopButton.attachPop(stopCallback);

  Serial.println("Initialization Complete");
  globalEnable(true);
  pinDownReleaseCallback(0);
  delay(1000);
}


void loop() {
  nexLoop(nex_listen_list); //process any waiting input from Nextion display

  if(digitalRead(xStop)) {
    stepperX.move(xPosAdjust);
  }
  if(digitalRead(yStop)){
    stepperY.move(yPosAdjust);
  }
  if(!stepperX.run()) {   //try to take a step.  
    if(testRunning) {  //If we can't, check if the test is running, clear out the test and reset if so
      testRunning = false; //mark test as finish
      globalEnable(true);
    }
  }
	stepperY.run();//will take a single step in the configured direction at the configured speed
}