
 
#include <ESP32Servo.h>
#include <Arduino.h>

class ServoMotor{
    uint32_t servoPin = 0;
    Servo servo;

public:
    ServoMotor(const uint32_t servoPin) : servoPin(servoPin){
    ESP32PWM::allocateTimer(1); // make sure to not interfere with the tone function
	servo.setPeriodHertz(50);    // standard 50 hz servo
	servo.attach(servoPin); // attaches the servo on pin 18 to the servo object
    }

    void open(){
        Serial.println("open");
        servo.write(180);
        delay(667);
        //servo.write(90);
        servo.release();
    }

    bool close(){
        Serial.println("close");
        //if(analogRead(33) > 2850){
            servo.write(0);
            delay(667);
            //servo.write(90);
            servo.release();
        return true;
        /*}else{
            return false;
        }*/
    }

};