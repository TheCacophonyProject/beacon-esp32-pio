#include <BLEAdvertisedDevice.h>

class PingBeacon {
    public:
        
        std::string rawData;
        int beaconType = 0x01;
        int deviceID;
        PingBeacon(std::string r) {
            rawData = r;
        };
};