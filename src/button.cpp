/*#include <arduino.h>

constexpr uint32_t upPin = 25, downPin = 16, selectPin = 18, cencelPin = 19;

void setup(){

    Serial.begin(9600);
    pinMode(upPin, INPUT_PULLUP);
    pinMode(downPin, INPUT_PULLUP);
    pinMode(selectPin, INPUT_PULLUP);
    pinMode(cencelPin, INPUT_PULLUP);

}

void loop(){
    if(digitalRead(upPin) == LOW){
        Serial.println("Up button pushed");
    }
    if(digitalRead(downPin) == LOW){
        Serial.println("down button pushed");
    }
    if(digitalRead(selectPin) == LOW){
        Serial.println("select button pushed");
    }
    if(digitalRead(cencelPin) == LOW){
        Serial.println("cencel button pushed");
    }
    

}*/