#pragma once
#include <HX711_ADC.h>


 void dataReadyISR(void* driver)
    {
        ((HX711_ADC*)driver)->update();
    }

class LoadCell
{
private:
    HX711_ADC HX711_driver;

public:
LoadCell(const int &dataPin, const int &clockPin, const float &calibrationValue, const int &samples = 16) : HX711_driver(dataPin, clockPin)
    {
        Serial.begin(9600);
        HX711_driver.begin();
        HX711_driver.setSamplesInUse(samples);

        if (HX711_driver.getTareTimeoutFlag())
        {
            Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
            while (1);
        }
        else
        {
            HX711_driver.setCalFactor(calibrationValue); // set calibration value (float)
            Serial.println("Startup is complete");
        }

        attachInterruptArg(digitalPinToInterrupt(dataPin), dataReadyISR, &HX711_driver, FALLING);
    }

    void start(const uint32_t &stabilizingTime = 2000){
        HX711_driver.start(stabilizingTime, false);
    }

    void tare()
    {
        HX711_driver.tare();
    }

    void setCalibrationFactor(const float &calibrationValue)
    { 
        HX711_driver.refreshDataSet();
        HX711_driver.getNewCalibration(calibrationValue);
    }

    float getData()
    {
        HX711_driver.update();
        return HX711_driver.getData();
    }
};