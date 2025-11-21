
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

enum class Menues{opening, timeLocker, selectTime, timeLocked, calorinLocker, inputMinCalorinsToOpen, inputFullCalorins, calorinsToGo, calibrate, tare, knownMass} currentMenu;
bool isLocked;
unsigned long timeSinceStartLocking;
unsigned long minutesLocked = 1;

//screens

//openingscreen
void openScreen() {
  if(isLocked){
    display.setText("Locked\nPress to unlock");
    selectButton.setOnPress([](){
      isLocked = false;
      servoMotor.open();
      buzzer.playRewardSound();
    });
  } else {
    display.setText("Open\nPress to unlock");
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
    Serial.println("Down pressed!");
    //buzzer.playNotificationSound();
  });
}

//timer locker screen
void timeLockerScreen(){
  display.setText("time locker");
  selectButton.setOnPress([](){
    currentMenu = Menues::selectTime;
  });

  upButton.setOnPress([](){
    currentMenu = Menues::opening;
  });

  downButton.setOnPress([](){
    currentMenu = Menues::calibrate;
  });
}

//TODO: make this more beautiful like in a class
namespace selectTimeVariables{
  unsigned long minutesLockedOnPress;
}


//select time to lock
  void setTimeScreen(){
    uint32_t hours = minutesLocked / 60;
    uint32_t minutes = minutesLocked % 60;
    String text = "lock for:\n" + String(hours) + ":" + String(minutes);
    display.setText(text);
    
    selectButton.setOnPress([](){
      timeSinceStartLocking = millis();
      servoMotor.close();
      currentMenu = Menues::timeLocked;
    });
    cancelButton.setOnPress([](){
      currentMenu = Menues::timeLocker;
    });

    downButton.setOnPress([](){
      if(minutesLocked > 1){
        minutesLocked-=1;
      }
    });

    downButton.setOnLongPressStart([](){
      selectTimeVariables::minutesLockedOnPress = minutesLocked;
      Serial.println("Long press start");
    });
    downButton.setOnLongPress([](double onLongPress){
      minutesLocked = selectTimeVariables::minutesLockedOnPress - onLongPress * 15;
      if(minutesLocked < 1){
        minutesLocked = 1;
      }
    });



  upButton.setOnPress([](){
    Serial.println(minutesLocked);
    minutesLocked+=1;
  });

      upButton.setOnLongPressStart([](){
      selectTimeVariables::minutesLockedOnPress = minutesLocked;
      Serial.println("Long press start");
      Serial.println(minutesLocked);

    });
    upButton.setOnLongPress([](double onLongPress){
      minutesLocked = selectTimeVariables::minutesLockedOnPress + onLongPress * 15;
    });
    
  }

void secondsLockedScreen(){
    unsigned long timeToGo = (minutesLocked * 60) - (millis() - timeSinceStartLocking) / 1000;

    if(timeToGo <= 0){
      servoMotor.open();
      currentMenu = Menues::timeLocker;
    }

    uint32_t hours = timeToGo / 3600;
    timeToGo -= hours * 3600;
    uint32_t minutes = timeToGo / 60;
    timeToGo -= minutes * 60;
    uint32_t seconds = timeToGo;
    String text = "locked for:\n" + String(hours) + ":" + String(minutes) + ":" + String(seconds);
    display.setText(text);

}

void calibrateScreen(){
  display.setText("Calibrate Weight");
  selectButton.setOnPress([](){
    currentMenu = Menues::tare;
  });

  upButton.setOnPress([](){
    currentMenu = Menues::timeLocker;
  });

  downButton.setOnPress([](){
    //currentMenu = Menues::calibrate;
  });
}

void setup(){
    Serial.begin(9600);
    display.begin();
    servoMotor.open();
    currentMenu = Menues::timeLocker;
}

void loop(){
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

  case Menues::timeLocker:
    timeLockerScreen();
    break;

  case Menues::selectTime:
    setTimeScreen();
  break;

  case Menues::timeLocked:
    secondsLockedScreen();
  break;

  case Menues::calibrate:
    calibrateScreen();
  break;
  /*case Menues::calorinLocker:
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
    break;

  default:
    break;*/
  }
}