//this program will look for a button press then it will push the solenoid out and in. 
#define IBTN 18
int motorPWM = 44;
bool p = 0;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(IBTN, INPUT);  
  pinMode(motorPWM, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(IBTN), push, FALLING);   
  digitalWrite(10,LOW);
  digitalWrite(11,LOW);
  
}

// the loop function runs over and over again forever
void loop() {
  if(p==1){
    analogWrite(motorPWM,250);

    digitalWrite(10,LOW);
    digitalWrite(11,HIGH);
    while((analogRead(0))<900){
      delay(10);
    }
    
    
    digitalWrite(10,LOW);
    digitalWrite(11,LOW);
        
    digitalWrite(10,HIGH);
    digitalWrite(11,LOW);
    while((analogRead(0))>100){
      delay(10);
    }
    

    p=0;  

  }
  analogWrite(motorPWM,10);

  delay(1);
}

void push() {
  p=1;
}

