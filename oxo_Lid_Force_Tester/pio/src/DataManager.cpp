#include "DataManager.h"


DataManager::DataManager() {
    //_count = 0;
}


void DataManager::init(void) {
    pinMode(_defaultChipSelectPin, OUTPUT);    // 53
    pinMode(_etherSlaveSelectPin, OUTPUT);     // 10
    //digitalWrite(_etherSlaveSelectPin, HIGH);  // Disable Ethernet chip
}


bool DataManager::sdPresent(void) {

    _enableSD();

    if (!SD.begin(_sdSlaveSelectPin)) {
        Serial.println("Did not begin");
        return false;
    } else {
        return true;
    }

    _disableSD();

}


void DataManager::writeData(void) {

    _enableSD();
    //NO MORE THAN 8 CHARACHTER FILE NAME + 3 FOR THE EXTENSION
    file = SD.open("dag333444.txt", FILE_WRITE);

    // if the file is available, write to it:
    if (file) {
        file.println("Test");
        file.close();
    // print to the serial port too:
    Serial.println("Test success");
    }
    // if the file isn't open, pop up an error:
    else {
        Serial.println("Error opening datalog.txt");
    }

    _disableSD();

}


void DataManager::writeData(bool m,float reading) {
    
    _enableSD();


    /////////////////////////////////////////////////////////////
    //NO MORE THAN 8 CHARACHTER FILE NAME + 3 FOR THE EXTENSION//
    /////////////////////////////////////////////////////////////

    if (m){
        file = SD.open("REMOTE.txt", FILE_WRITE);  //effectivvely append mode
    }
    else{
        file = SD.open("LOCAL.txt", FILE_WRITE); //effectivvely append mode
    }
    
   
    // if the file is available, write to it:
    if (file) {
        file.println(reading);
        file.close();
        // print to the serial port too:
        Serial.println("Writing to SD Card:");
        Serial.println(reading);
    }
    // if the file isn't open, pop up an error:
    else {
        Serial.println("Error opening datalog.txt");
    }
    
    _disableSD();

}


void DataManager::readData(void) {
    ;
}


void DataManager::_enableSD(void) {
    digitalWrite(_sdSlaveSelectPin, LOW);
    digitalWrite(_etherSlaveSelectPin, HIGH);  // Disable Ethernet chip
}


void DataManager::_disableSD(void) {
    digitalWrite(_sdSlaveSelectPin, HIGH);       // Disable SD Card
    digitalWrite(_etherSlaveSelectPin, LOW);    // Enable Ethernet chip
}