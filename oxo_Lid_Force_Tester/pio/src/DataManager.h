#include <Arduino.h>
#include "SPI.h"
#include "SD.h"

class DataManager {
    public:
    DataManager();
    void init(void);
    bool sdPresent(void);
    void writeData(void);
    void readData(void);
    void writeData(bool m, float reading);

    private:
    ///////////////////////////////////////////////////////////////////////////////
    // _chipSelectPin must be set to 4
    // This is what adafruit ethernet shield is using as chip select pin.
    // _etherSlaveSelect is pin 10 and is what is used to chip select the 
    // ehternet IC.  This line needs to be set to HIGH in order to communicate to
    // SD card.
    // _defaultChipSelect is default SPI chip select pin on MEGA2560, it must be
    // set to HIGH in order for SPI to work, even though a different pin is used
    // for chip select.
    ///////////////////////////////////////////////////////////////////////////////
    const int _sdSlaveSelectPin = 4;
    const int _etherSlaveSelectPin=10;
    const int _defaultChipSelectPin=53;
    File file;
    void _enableSD(void);
    void _disableSD(void);
};