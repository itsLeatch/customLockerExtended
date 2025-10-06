#pragma once

#ifndef LONGPRESSTIME
    //define the long press time in seconds
    #define LONGPRESSTIME 0.75 
#endif

#ifndef REBOUNCETIME
    //sometimes the button fires multime presses because of rebounce --> inputs in this small time intervals will be ignored
    #define REBOUNCETIME 0.05 
#endif

#include <Arduino.h>

class Button{
private:
    unsigned int pin;
    void (*onPress) ();
    void (*onLongPress) (double timeSincePress);

    double pressStartedTime;
    bool isPressed = false;
    public:
    Button(unsigned int pin = 0, void (*onPress) () = {}, void (*onLongPress) (double timeSincePress) = {}) : pin(pin), onPress(onPress), onLongPress(onLongPress){
        pinMode(pin,INPUT_PULLUP);
    }

    unsigned int getPin() const {
        return pin;
    }

    void setPin(const unsigned int &newPin) {
        pin = newPin;
        pinMode(pin, INPUT_PULLUP);
    }

    void setOnPress(void (*callback) ()) {
        onPress = callback;
    }

    void setOnLongPress(void (*callback) (double)) {
        onLongPress = callback;
    }

    void update(){
          if(digitalRead(pin) == LOW){
        //when switching from non pressed to pressed
        if(isPressed == false){
            pressStartedTime = (double) millis() / 1000.0;
            isPressed = true;
        }
        double timeSincePress = (millis () /1000.0) - pressStartedTime;
        if(timeSincePress >= LONGPRESSTIME){
            onLongPress(timeSincePress - LONGPRESSTIME);
        }

    }else{
        //when released
        if(isPressed == true){
            isPressed = false;
            double timeSincePress = (millis () /1000.0) - pressStartedTime;
            //when the button was not long pressed
            if(timeSincePress < LONGPRESSTIME && timeSincePress > REBOUNCETIME){
                onPress();
            }
        }
    
    }
    }
};