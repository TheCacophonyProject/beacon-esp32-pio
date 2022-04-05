#ifndef cac_beacons
#define cac_beacons

#include <BLEAdvertisedDevice.h>
#include "cacBeacons.h"

PingBeacon::PingBeacon(std::string r) {
    rawData = r;
}

ClassificationBeacon::ClassificationBeacon(std::string r) {
    rawData = r;
    uint8_t data[100];
    rawData.copy((char *)data, rawData.length(), 0);
    deviceID = (data[3]<<8) + data[4];
    for (int i = 0; i < data[6]; i++) {
        animal[i] = data[i+7];
        confidences[i] = data[i+8];
    }
}





/*
class PingBeacon {
    public:
        std::string rawData;
        int deviceID;
        PingBeacon(std::string r) {
            rawData = r;
        };
};


class RecordingBeacon {
    public:
        std::string rawData;
        int deviceID;
        RecordingBeacon(std::string r) {
            rawData = r;
        };
};

class ClassificationBeacon {
    public:
        std::string rawData;
        int deviceID;
        ClassificationBeacon(std::string r) {
            rawData = r;
        };
};

class PowerOffBeacon {
    public:
        std::string rawData;
        int deviceID;
        PowerOffBeacon(std::string r) {
            rawData = r;
        };
};
*/

#endif