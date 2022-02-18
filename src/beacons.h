#ifndef beacons_h
#define beacons_h
#include <BLEAdvertisedDevice.h>

class MyAdvertisedDeviceCallbacks : BLEAdvertisedDeviceCallbacks {
    public:
        void onResult(BLEAdvertisedDevice);
};



#endif