// This is the Lid Force Tester.
//
// Version 1 
//
// Setup
// Arduino Mega 2560
// HX711a
// Serial LCD from SparkFun

#include <Arduino.h>
#include "HX711.h"
#include "serLCD.h"
#include "DataManager.h"
#include "PubSubClient.h"
#include "Ethernet2.h"

///////////////////////////////////////////////////////////////////////////////
// Prototypes
///////////////////////////////////////////////////////////////////////////////
void ISR_BUTTON(void);
ISR(TIMER5_COMPA_vect);
void timerA_setup(void);

void callBack(char* topic, byte* payload, unsigned int length);
void sub(char* topic);
void reconnect(void);
void pub(char* topic);
void pubFloat(char* topic, float data);
void ethernetConnect();

///////////////////////////////////////////////////////////////////////////////
// Define pins
///////////////////////////////////////////////////////////////////////////////
#define MODE_PIN 2  // Switch for choosing measurement or calibration mode
#define LCD_RX_PIN 8 // RX pin
#define INTERRUPT_1_PIN 3 //Button interrup pin
#define TEST_WINDOW_TIME_SECONDS 15

///////////////////////////////////////////////////////////////////////////////////
// Topics constants
///////////////////////////////////////////////////////////////////////////////////
#define ADAFRUIT_USERNAME               "npentelovitch"
#define FEED_PATH ADAFRUIT_USERNAME     "/feeds/lid-force-measurement"
#define FEED_PATH2 ADAFRUIT_USERNAME    "/feeds/lid-force-measurement-local-mode"


///////////////////////////////////////////////////////////////////////////////////
// Ethernet setup
///////////////////////////////////////////////////////////////////////////////////
byte mac[] = { 0x90, 0xA2, 0xDA, 0x11, 0x19, 0xBC }; 
    // This is the address of the ethernet board. It's specific to the ethernet shield

///////////////////////////////////////////////////////////////////////////////////
// MQTT Server setup
///////////////////////////////////////////////////////////////////////////////////
char mqttServerURL[] =          "io.adafruit.com";
int mqttServerPort= 1883;   // This is port that the mqtt protocol uses. 
                            // AKA port 80 http or 443 for https 

// Subscribe to arduino/control topic
char arduinoDataTopic[] = FEED_PATH;
char arduinoDataTopic2[] = FEED_PATH2; 
char arduinoControlTopic[] =    "npentelovitch/feeds/lid-force-measurement";


char clientID[] =               "npentelovitch";
char mosquittoUser[] =          "npentelovitch";
char mosquittoAuth[] =          "e460fc5971c2416d84bcd3d6f98515a7"; //aka AIO Key

///////////////////////////////////////////////////////////////////////////////////
// Ethernet client instance
///////////////////////////////////////////////////////////////////////////////////
EthernetClient ethClient;

///////////////////////////////////////////////////////////////////////////////////
// mqtt client instance
///////////////////////////////////////////////////////////////////////////////////
PubSubClient mqttClient(mqttServerURL, mqttServerPort, callBack, ethClient);

///////////////////////////////////////////////////////////////////////////////
// Define scale amp data pins
///////////////////////////////////////////////////////////////////////////////
#define DOUT SDA
#define SCL1 SCL

///////////////////////////////////////////////////////////////////////////////
// Define scale related variables.
///////////////////////////////////////////////////////////////////////////////
volatile float reading = 0;
volatile float r = 0;

///////////////////////////////////////////////////////////////////////////////
// Define Time for Max of 15s of recording time 
///////////////////////////////////////////////////////////////////////////////
unsigned long t1;
unsigned long t2;

///////////////////////////////////////////////////////////////////////////////
// Assign mode pin
///////////////////////////////////////////////////////////////////////////////
uint32_t switchPin = MODE_PIN;

///////////////////////////////////////////////////////////////////////////////
// mode is used to run the program either in local or
// remote mode.  This avoids having to run two separate
// sketches.
///////////////////////////////////////////////////////////////////////////////
bool mode = true; // True mode is remote mode.

///////////////////////////////////////////////////////////////////////////////
// Variables for aquiring scale readings and calculating max reading.
///////////////////////////////////////////////////////////////////////////////
// char readingChar[8]; can output to LCD directly
float setForce = 1.0;
bool collectData = false; //Flag for indicating if data has been collected
float readingsArray[1000]; //Array for capturing readings
int count = 0;  //counter for iterating through arrays or readings
volatile bool buttonPressed = false;
unsigned long waitPeriodMs = 0;

bool ethernetAvailable = false;

///////////////////////////////////////////////////////////////////////////////
// mode is used to run the program either in measurement or
// calibration mode.  This avoids having to run two separate
// sketches.
///////////////////////////////////////////////////////////////////////////////
HX711 scale(DOUT, SCL1);

///////////////////////////////////////////////////////////////////////////////
// Define calibration factor.  This can be adjusted in calibration mode by
// pressing a/z or +/- keys
///////////////////////////////////////////////////////////////////////////////
float calibration_factor = -210000;

///////////////////////////////////////////////////////////////////////////////
// Instantiate lcd screen
///////////////////////////////////////////////////////////////////////////////
serLCD lcd = serLCD(LCD_RX_PIN);


///////////////////////////////////////////////////////////////////////////////
// SD Card data manager.
// This uses: pin 10, pin 53 and pin 4.
// Make sure these pins are not used for anything else.
///////////////////////////////////////////////////////////////////////////////
DataManager dm = DataManager();

///////////////////////////////////////////////////////////////////////////////
// Setup
///////////////////////////////////////////////////////////////////////////////
void setup() {
    
    ///////////////////////////////////////////////////////////////////////////
    // Halt program execution until serial port is connected.
    ///////////////////////////////////////////////////////////////////////////
    Serial.begin(9600);
    // while(!Serial) {
    //     ; //Wait for Serial connection to start up.
    // }
    

    ///////////////////////////////////////////////////////////////////////////
    // Setup timer A
    ///////////////////////////////////////////////////////////////////////////
    timerA_setup();

    ///////////////////////////////////////////////////////////////////////////
    // Setup interrupt
    ///////////////////////////////////////////////////////////////////////////
    attachInterrupt(1, ISR_BUTTON, FALLING);

    ///////////////////////////////////////////////////////////////////////////
    // Setup LCD screen.
    ///////////////////////////////////////////////////////////////////////////
    lcd.setType(3); // this sets it to a 20 char lcd 
    lcd.setType(5); // this sets it to a 4 row with offset of 1 
    lcd.clear();
    lcd.setCursor(1,1);
    lcd.print("LCD OK");
    delay(1000);
    lcd.clear();
    
    ///////////////////////////////////////////////////////////////////////////
    // Set mode on switch pin.
    ///////////////////////////////////////////////////////////////////////////
    pinMode(switchPin, INPUT);

    ///////////////////////////////////////////////////////////////////////////
    // Select mode of operation.
    ///////////////////////////////////////////////////////////////////////////
    if (digitalRead(switchPin)==HIGH) {
        lcd.setCursor(1,1);
        lcd.print("Remote Logging mode");
        scale.set_scale(calibration_factor);        //This value is obtained by using the SparkFun_HX711_Calibration sketch
        scale.tare();                               //Assuming there is no weight on the scale at start up, reset the scale to 0
        delay(1500);
        lcd.clear();
    } else {
        lcd.setCursor(1,1);
        lcd.print("Local Mode");
        mode = false;
        scale.set_scale(calibration_factor);
        scale.tare();                               // Reset the scale to 0
        //long zero_factor = scale.read_average();  // Get a baseline reading
        //Serial.print("Zero factor: ");            // This can be used to remove the need to tare the scale. Useful in permanent scale projects.
        //Serial.println(zero_factor);
        lcd.setCursor(2,1);
        //lcd.print("Zero factor");
        //lcd.setCursor(3,1);
        //lcd.print(zero_factor);
        delay(1500);
        lcd.clear();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Init DataManager
    ///////////////////////////////////////////////////////////////////////////    
    dm.init();
    if (dm.sdPresent()) {
        lcd.setCursor(1,1);
        lcd.print("SD Card Ready");
        dm.writeData();
        
    } else {
        lcd.setCursor(1,1);
        lcd.print("No SD Card");        
    }
    delay(1000);

    ///////////////////////////////////////////////////////////////////////////////
    // Ethernet
    ///////////////////////////////////////////////////////////////////////////////
    ethernetConnect(); // function implemented below
    

    t1 = millis();
    delay(20);
    t2 = millis();

}

///////////////////////////////////////////////////////////////////////////////
// Main loop
///////////////////////////////////////////////////////////////////////////////
void loop() {

    if (ethernetAvailable == true) {
        if (!mqttClient.connected()){
            reconnect();
        }
        mqttClient.loop(); 
    }
    
    if (mode) {
        // This is remote mode
        lcd.setCursor(1,1);
        lcd.print("Ready:");
        reading = scale.get_units();

        if(reading<0){
            reading *=(-1);//absoute value
        }
        lcd.setCursor(2,1);
        lcd.print(reading);
        lcd.setCursor(2, 6);
        lcd.print("kg");
                 
        if (waitPeriodMs !=0) {
            lcd.setCursor(4,1);
            lcd.print((waitPeriodMs/1000)); 
        } else {
            delay(200);
            lcd.clearLine(4);
        }

        // Wait for force to exceed set level & btn is pressed and capture readings.
        // Set data collection flag to true
        // Set buttonPressed flag to false 

        if ( (reading > setForce)&& (buttonPressed == true) ) {
            lcd.clearLine(4);
            waitPeriodMs = 0;

            lcd.setCursor(3,1);
            lcd.print("Capturing Data.");
            collectData = true;
            lcd.clearLine(2);
            r = reading;

            while(count < 1000 && r>setForce) {
                r = scale.get_units();
                if(r<0){
                    r *=(-1);//absoute value
                }
                readingsArray[count] = r;
                count++;
                //this will never overflow/ overwrite
            }
            reading = r;
        }

        // Once readigns are taken and force lowers below set value
        // calculate maximum captured force.
        if ( (collectData == true)  ) {//&& (reading < setForce)

            float maximum = 0;
            for( int i=0; i <= 999; i++) {
                if ( readingsArray[i] > maximum){
                    maximum = readingsArray[i];
                }
            }
            collectData = false;
            buttonPressed = false;

            lcd.clearLine(3);
            lcd.setCursor(3,1);
            lcd.print("MAX: ");
            lcd.setCursor(3,5);
            lcd.print(maximum, 2);
            delay(1000);

            //Write data to sd card
            dm.writeData(mode,maximum);
            if (ethernetAvailable == true) {
                pubFloat(arduinoDataTopic, maximum);
                
            }
            

            for( int i=0; i <=999; i++) {
                readingsArray[i] = 0;
            }
            count = 0;
        }
        
    } else {
        // This is Local mode

        lcd.setCursor(1,1);
        lcd.print("Ready:");
        reading = scale.get_units();
        

        if(reading<0){
            reading *=(-1);//absoute value
        }
        lcd.setCursor(2,1);
        lcd.print(reading);
        lcd.setCursor(2, 6);
        lcd.print("kg");
                 
        if (waitPeriodMs !=0) {
            lcd.setCursor(4,1);
            lcd.print((waitPeriodMs/1000)); 
        } else {
            lcd.clearLine(4);
        }

        // Wait for force to exceed set level & btn is pressed and capture readings.
        // Set data collection flag to true
        // Set buttonPressed flag to false 

        if ( (reading > setForce)&& (buttonPressed == true) ) {
            lcd.clearLine(4);
            waitPeriodMs = 0;

            lcd.setCursor(3,1);
            lcd.print("Capturing Data.");
            collectData = true;
            lcd.clearLine(2);
            r = reading;

            while(count < 1000 && r>setForce) {
                r = scale.get_units();
                if(r<0){
                    r*=(-1);//absoute value
                }
                readingsArray[count] = r;
                count++;
                //this will never overflow/ overwrite
            }
            reading = r;
        }

    
    

        // Once readings are taken and force lowers below set value
        // calculate maximum captured force.
        if ( (collectData == true)  ) {//&& (reading < setForce)
            // loop all the values
            float maximum = 0;
            for( int i=0; i <= 999; i++) {
                if ( readingsArray[i] > maximum){
                    maximum = readingsArray[i];
                }
            }
            //reset flags
            collectData = false;
            buttonPressed = false;

            //display max
            lcd.clearLine(3);
            lcd.setCursor(3,1);
            lcd.print("MAX: ");
            lcd.setCursor(3,5);
            lcd.print(maximum, 2);
            delay(1000);

            //Write data to sd card
            dm.writeData(mode,maximum);
            if (ethernetAvailable == true) {//publish to the secret topic
                pubFloat(arduinoDataTopic2, maximum);
            }
            
            //reset array
            for( int i=0; i <=999; i++) {
                readingsArray[i] = 0;
            }
            count = 0;
        }

    }//end of local mode
}//void loop

void ethernetConnect(){
    lcd.clear();
    lcd.setCursor(1,1);
    if(mode){
        lcd.print("Trying DHCP");
    }else{
        lcd.print("Setting Up");
    }
    
    delay(500);
    
    if (Ethernet.begin(mac) == 1) {
        ethernetAvailable = true;
        lcd.setCursor(2,1);
        if (mode){
            lcd.print("Ethernet available");
        }
        else{
            lcd.print("...");  
        }
        delay(1000);
        lcd.clear();
    } else {
        lcd.clear();
        lcd.setCursor(1,1);
        lcd.print("No ethernet avail.");
        lcd.setCursor(2,1);
        lcd.print("Running w/o mqtt");
        delay(1000);
        lcd.clear();
    }
    

    


    if (ethernetAvailable == true && mode ) {
        //remote mode
        lcd.clear();
        lcd.setCursor(1,1);
        lcd.print("IP: ");
        lcd.setCursor(2,1);
        lcd.print(Ethernet.localIP());
        delay(1000);
        lcd.clear();

        ///////////////////////////////////////////////////////////////////////////////
        // MQTT                                                                      //
        ///////////////////////////////////////////////////////////////////////////////
        mqttClient.setServer(mqttServerURL, mqttServerPort);
        mqttClient.connect(clientID, mosquittoUser, mosquittoAuth);
        
        lcd.setCursor(1,1);
        lcd.print("MQTT status: ");
        lcd.setCursor(2,1);
        lcd.print(mqttClient.state());
        delay(500);
        
        sub(arduinoDataTopic);

        lcd.setCursor(3,1);
        lcd.print("Subscribed to: ");
        lcd.setCursor(4,1);
        lcd.print(arduinoDataTopic);
        delay(1000);
        lcd.clear();
        
        
    } 
    else if ( ethernetAvailable == true && !mode){
        //local mode
        mqttClient.setServer(mqttServerURL, mqttServerPort);
        mqttClient.connect(clientID, mosquittoUser, mosquittoAuth);
        
        delay(500);
        sub(arduinoDataTopic2);
        
        lcd.setCursor(3,1);
        lcd.print("Ready");
        delay(1000);
        lcd.clear();
    }

}



///////////////////////////////////////////////////////////////////////////////
// Interrupt 3(20) service rutine.
///////////////////////////////////////////////////////////////////////////////
void ISR_BUTTON() {
    buttonPressed = true;
    lcd.clearLine(3);
}


///////////////////////////////////////////////////////////////////////////////
// Timer A setup
///////////////////////////////////////////////////////////////////////////////
void timerA_setup() {
    
    noInterrupts();

    // Setting PL3 as output
    // to get Capture Compare interrupt pin 
    // to toggle state.
    // DDRL = 0b0001000;
    DDRL = (1<<DDL3);


    ///////////////////////////////////////////////////////////////////////////
    // Set timer5 to 10 Hz with interrupt (see ISR(TIMER_COMPA_vect))
    ///////////////////////////////////////////////////////////////////////////
    TCCR5A = 0; // Set entire register to 0
    TCCR5B = 0; // Set entire register to 0
    TCNT5 = 0;  // Init counter value to 0 (This is where actual value of the time is stored.)

    // Set compare/match registers for 10 Hz
    OCR5A = 1561;  // ( (16*10^6) / (10*1024) ) -1
    // Enable CTC mode
    TCCR5B |= (1<<WGM52);
    // Set CS50 CS52 to set prescaler to 1024
    TCCR5B |= (1<<CS52)|(1<<CS50);
    // Enable timer compare interrupt
    TIMSK5 |= (1<<OCIE5A);

    //enable pin toggle
    //TCCR5A |= (1<<COM5A0)|(1<<COM5A1);
    TCCR5A |= (1<<COM5A0);

    interrupts();
}


ISR(TIMER5_COMPA_vect) {
    if ( (buttonPressed == true) && ( collectData == false ) ) {
        waitPeriodMs+=100;
        if ( (waitPeriodMs/1000) >= TEST_WINDOW_TIME_SECONDS ) {
            buttonPressed = false;
            waitPeriodMs = 0;
        } 
    }       
}

///////////////////////////////////////////////////////////////////////////////
// Mqtt callback
///////////////////////////////////////////////////////////////////////////////
void callBack(char* topic, byte* payload, unsigned int length) {
    if (mode){
        Serial.print("New Message -> ");
        Serial.print(topic);
        Serial.print(": [");
        
        for (unsigned int i=0;i<length;i++) {
            Serial.print( (char)payload[i]);
        }
        Serial.println("]");
    }
}


///////////////////////////////////////////////////////////////////////////////
// Subscribe
///////////////////////////////////////////////////////////////////////////////
void sub(char* topic) {
    mqttClient.subscribe(topic);
}


///////////////////////////////////////////////////////////////////////////////
// Publish
///////////////////////////////////////////////////////////////////////////////
void pub(char* topic) {
    mqttClient.publish(topic, "Test" );
}


///////////////////////////////////////////////////////////////////////////////
// Publish a float
///////////////////////////////////////////////////////////////////////////////
void pubFloat(char* topic, float data) {
    char result[8]; // Buffer big enough for 7-character float
    dtostrf(data, 5, 2, result); // Leave room for too large numbers!
    mqttClient.publish(topic, result );
}


///////////////////////////////////////////////////////////////////////////////
// Reconnect in case connection is droppped.
///////////////////////////////////////////////////////////////////////////////
void reconnect(void) {
    lcd.clear();
    lcd.setCursor(0,0);
    if (mode){lcd.print("Lost MQTT");}
    lcd.setCursor(0,1);
    if (mode){lcd.print("connection");}
    delay(500);
    lcd.clear();
    while (!mqttClient.connected()) {
        lcd.setCursor(0,0);
        if (mode){
            lcd.print("Reconnecting");
            delay(900);
        }
        delay(100);
        if (mqttClient.connect(clientID, mosquittoUser, mosquittoAuth)) {
            lcd.clear();
            if (mode){
                lcd.print("Reconnected");
                delay(900);
            }
            delay(100);
            if (mode){sub(arduinoDataTopic);}
            else{sub(arduinoDataTopic2);}
            delay(500);
            lcd.clear();
        } else {
            lcd.clear();
            lcd.setCursor(1,1);
            if (mode){
                lcd.print("Reconnect failed");
                delay(1000);
                lcd.setCursor(2,1);
                lcd.print(" rc=");
                lcd.setCursor(2,6);
                lcd.print(mqttClient.state());
                lcd.setCursor(3,1);
                lcd.print("Retrying in");
                lcd.setCursor(4,1);
                lcd.print("10 sec");
                delay(10000);
                lcd.clear();
            }
            else{
                lcd.print("SD Error");
                lcd.setCursor(2,1);
                lcd.print("Retrying in");
                lcd.setCursor(3,1);
                lcd.print("10 sec");
                delay(10000);
                lcd.clear();
            }            
            
        }       
    }
}