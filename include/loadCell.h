#pragma once
#include <HX711_ADC.h>


class LoadCell : public HX711_ADC
{

public:
LoadCell(const int &dataPin, const int &clockPin, const float &calibrationValue, const int &samples = 16) : HX711_ADC(dataPin, clockPin)
    {
        Serial.begin(9600);
        begin();
        setSamplesInUse(samples);

        if (getTareTimeoutFlag())
        {
            Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
            while (1);
        }
        else
        {
            setCalFactor(calibrationValue); // set calibration value (float)
            Serial.println("Startup is complete");
        }

    }

    void calibrateWithKnowMass(const float &knowMass)
    { 
        refreshDataSet();
        getNewCalibration(knowMass);
    }

    float getData()
    {
        update();
        return HX711_ADC::getData();
    }
};