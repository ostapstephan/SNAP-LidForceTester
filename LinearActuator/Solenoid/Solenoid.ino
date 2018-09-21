//this program will look for a button press then it will push the solenoid out and in. 
// Tested and working with the arduino mega and the board that Eddy made. 
//[red black black red] for the wiring into the [vin, gnd, 1a, 1b] on the motor controller board
#define ISB 45
#define ISA 46
#define IBTN 18
int motorPWM = 44;
bool p = 0;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(IBTN, INPUT);  
  pinMode(motorPWM, OUTPUT);
  pinMode(ISA, OUTPUT);
  pinMode(ISB, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(IBTN), push, FALLING);   
  digitalWrite(ISA,LOW);
  digitalWrite(ISB,LOW);
  Serial.begin(9600);
  
}

// the loop function runs over and over again forever


void loop() {
  if(p==1){
    analogWrite(motorPWM,250);
    delay(10);
    
    pushout();                      // this is an statement that should be true for     <---------900
    while((analogRead(0))<900){     // ensure that it reaches the open position ie pot reads more than 900
      Serial.println(analogRead(0));
      delay(10);
    }
    
    pushin();
    delay(10);                      // this is an statement that should be true for     100 ---------> 
    while((analogRead(0)>100)){     // ensure that it reaches the closed position ie pot reads less than 100
      Serial.println(analogRead(0));
      delay(10);
    }
    
    digitalWrite(ISA,LOW);
    digitalWrite(ISB,LOW);
    p=0;  //RESET THE BUTTON
  }
  
  analogWrite(motorPWM,4);
  delay(10);
}


void push() {
  p=1;
  Serial.print("hi");
}

void pushout(){
  digitalWrite(ISA,LOW);
  digitalWrite(ISB,LOW);
  delay(10);
  digitalWrite(ISA,HIGH);//out
  digitalWrite(ISB,LOW);
}

void pushin(){
  digitalWrite(ISA,LOW);
  digitalWrite(ISB,LOW);
  delay(10);
  digitalWrite(ISA,LOW);//out
  digitalWrite(ISB,HIGH);
}


