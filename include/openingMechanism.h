// A open function, that spins a Servomotor a full Revolution to close something. This only works if the hall effect sensor detects a magned --> the box is closed. There are Variables SERVOMOTORPIN, HALLEFFECTPIN, HALLTHREASHOLD. The function is called close.
#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>

#ifndef SERVOMOTORPIN
    #define SERVOMOTORPIN 14 // Default pin for the servo motor
#endif

#ifndef HALLEFFECTPIN
    #define HALLEFFECTPIN 2 // Default pin for the hall effect sensor
#endif

#ifndef HALLTHRESHOLD
    #define HALLTHRESHOLD 100 // Default threshold for the hall effect sensor
#endif

// open method
void open() {
    Servo servo;
    servo.attach(SERVOMOTORPIN);
    servo.write(0); // Move servo to 0 degrees to open
    delay(1000); // Wait for the servo to reach the position
    servo.detach();
}

// close method that checks the hall effect sensor
void close() {
    Servo servo;
    servo.attach(SERVOMOTORPIN);
    
    // Check the hall effect sensor
    int hallValue = analogRead(HALLEFFECTPIN);
    if (hallValue < HALLTHRESHOLD) {
        Serial.println("Closing mechanism...");
        servo.write(360); // Move servo to 360 degrees to close
        delay(1000); // Wait for the servo to reach the position
    } else {
        Serial.println("Hall effect sensor not triggered, cannot close.");
    }
    
    servo.detach();
}
