#ifndef cac_beacons_h
#define cac_beacons_h
#include <BLEAdvertisedDevice.h>

#define BEACON_PING_TYPE 0x01
#define BEACON_RECORDING_TYPE 0x02
#define BEACON_CLASSIFICATION_TYPE 0x03
#define BEACON_POWER_OFF_TYPE 0x04


class CacBeaconCallback : BLEAdvertisedDeviceCallbacks {
    public:
        void onResult(BLEAdvertisedDevice);
};

class PingBeacon {
    public:
        std::string rawData;
        int deviceID;
        PingBeacon(std::string rawData);
};

class RecordingBeacon {
    public:
        std::string rawData;
        int deviceID;
        RecordingBeacon(std::string rawData);
};

class ClassificationBeacon {
    public:
        std::string rawData;
        int deviceID;
        int animal[5];
        int confidences[5];
        ClassificationBeacon(std::string rawData);
};

class PowerOffBeacon {
    public:
        std::string rawData;
        int deviceID;
        int minutesToSleep;
        PowerOffBeacon(std::string rawData);
};

#endif