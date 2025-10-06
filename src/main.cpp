
#include <Arduino.h>
#include "Buzzer.h"
#include "Display.h"
#include "Button.h"
#include "openingMechanism.h"
#include "ServoMotor.h"

//Komponents
Buzzer buzzer(5);
Display display(128, 64, -1); 
ServoMotor servoMotor(14);

Button selectButton(18,[](){
}, [](double timeSincePress){});
Button cancelButton(19,[](){
}, [](double timeSincePress){});

Button upButton(25,[](){
}, [](double timeSincePress){});

Button downButton(16,[](){
}, [](double timeSincePress){});

enum class Menues{opening, timeLocker, selectTime, secondsLocked, calorinLocker, inputMinCalorinsToOpen, inputFullCalorins, calorinsToGo, calibrate, tare, knownMass} currentMenu;
bool isLocked;

//screens

//openingscreen
void openScreen() {
  if(isLocked){
    display.setHeadline("Locked");
    display.setSubline("Press to  unlock");
    selectButton.setOnPress([](){
      isLocked = false;
      servoMotor.open();
      buzzer.playRewardSound();
    });
  } else {
    display.setHeadline("Open");
    display.setSubline("Press to  lock");
    selectButton.setOnPress([](){
      if(servoMotor.close()){
        buzzer.playNotificationSound();
        isLocked = true;
      }else{
        buzzer.playError();
      }
    });
}

  downButton.setOnPress([](){
    currentMenu = Menues::timeLocker;
    buzzer.playNotificationSound();
  });
}

void setup(){
    Serial.begin(9600);
    display.begin();

}

void loop(){
  Serial.println(analogRead(33));
    //update parts
    selectButton.update();
    cancelButton.update();
    upButton.update();
    downButton.update();

    switch (currentMenu)
  {
  case Menues::opening:
    openScreen();
  break;

  /*case Menues::timeLocker:
    timeLockerScreen();
    break;

  case Menues::selectTime:
    setTimeScreen();
  break;

  case Menues::secondsLocked:
    secondsLockedScreen();
  break;

  case Menues::calorinLocker:
    calorinLockerScreen();
    break;

  case Menues::inputMinCalorinsToOpen:
    inputMinCalorinsToOpenScreen();
    break;

  case Menues::inputFullCalorins:
    inputFullCalorinsScreen();
    break;

  case Menues::calorinsToGo:
    calorinsToGoScreen();
    break;

  case Menues::calibrate:
    calibrate();
    break;

  case Menues::tare:
    tare();
    break;

  case Menues::knownMass:
    knownMass();
    break;*/

  default:
    break;
  }
}