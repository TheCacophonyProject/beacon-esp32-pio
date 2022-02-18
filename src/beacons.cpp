#include "beacons.h"
#include <BLEAdvertisedDevice.h>

void MyAdvertisedDeviceCallbacks::onResult(BLEAdvertisedDevice advertisedDevice)
{
    if (advertisedDevice.haveManufacturerData() == true)
    {
    std::string strManufacturerData = advertisedDevice.getManufacturerData();
    
    uint8_t cManufacturerData[100];
    strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);

    if (strManufacturerData.length() >= 10 && strManufacturerData[0] == 0x12 && strManufacturerData[1] == 0x12)
    {
        //Serial.println("Found Cacophony beacon!");
        for (int i = 0; i < strManufacturerData.length(); i++)
        {
        //Serial.printf("[%X]", cManufacturerData[i]);
        }
        //Serial.printf("\n");
        //pBLEScan->stop();
        //Serial.println("Stopping scan.");
    }
    }
};