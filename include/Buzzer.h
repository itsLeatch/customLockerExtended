//A passive Buzzer configured as INPUT. That can Play notification an revard Sound as method.
#pragma once
#include <Arduino.h>
class Buzzer {
private:
    unsigned int pin;
    unsigned long startTime = 0;
    unsigned long duration = 0;
    unsigned int frequenze = 0; 

    void setTone(unsigned long toneDuration, unsigned int toneFrequenze){
        startTime = millis();
        duration = toneDuration;
        frequenze = toneFrequenze;
    }

public:
    Buzzer(unsigned int pin = 0) : pin(pin) {
        pinMode(pin, OUTPUT);
    }   
    unsigned int getPin() const {
        return pin;
    }
    void setPin(const unsigned int &newPin) {
        pin = newPin;
        pinMode(pin, OUTPUT);
    }
    //passive buzzer notification melody
    void playNotificationSound() {
        tone(pin,800,400);
    }
    //passive buzzer reward melody
    void playRewardSound() {
        tone(pin,1500,100);
    }

    void playError(){
        tone(pin,400,300);
        delay(300);
        tone(pin,1000,300);
        delay(300);
        tone(pin,1800,300);
    }

    void playNoTone(){
        noTone(pin);
    }

    
};

