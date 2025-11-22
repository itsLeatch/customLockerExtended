
#include <Arduino.h>
#include <EEPROM.h>

#include "Buzzer.h"
#include "Display.h"
#include "Button.h"
#include "openingMechanism.h"
#include "ServoMotor.h"
#include "loadCell.h"

// Komponents
Buzzer buzzer(5);
Display display(128, 64, -1);
ServoMotor servoMotor(14);
LoadCell loadCell(4, 27, 696.0);

Button selectButton(18, []() {}, [](double timeSincePress) {});
Button cancelButton(19, []() {}, [](double timeSincePress) {});

Button upButton(25, []() {}, [](double timeSincePress) {});

Button downButton(16, []() {}, [](double timeSincePress) {});

enum class Menues
{
  opening,
  timeLocker,
  selectTime,
  timeLocked,
  calorinLocker,
  inputMinCalorinsToOpen,
  inputFullCalorins,
  calorinsToGo,
  calibrate,
  tare,
  knownMass
} currentMenu;
bool isLocked;
unsigned long timeSinceStartLocking;
unsigned long minutesLocked = 1;
unsigned long controllMassInGramm = 0;

class State
{
private:
  float tareFactor;
  long tareOffset;

  void save()
  {
    EEPROM.put(0, *this);
    EEPROM.commit();
  }

  void readValues()
  {
    EEPROM.get(0, *this);
  }

public:
  void setTareFactor(const float &newTareFactor)
  {
    tareFactor = newTareFactor;
    save();
  }

  float getTareFacotor()
  {
    readValues();
    // make sure the tare factor is not nan or 0
    if (!(tareFactor > 0.0))
    {
      tareFactor = 1;
    }

    return tareFactor;
  }

  void setTareOffset(const long& newTareOffset){
    tareOffset = newTareOffset;
    save();
  }

  long getTareOffset(){
    readValues();

    if(!(tareOffset >= 0)){
      tareOffset = 0;
    }
    return tareOffset;
  }
} state;

// screens

// openingscreen
void openScreen()
{
  if (isLocked)
  {
    display.setText("Locked\nPress to unlock");
    selectButton.setOnPress([]()
                            {
      isLocked = false;
      servoMotor.open();
      buzzer.playRewardSound(); });
  }
  else
  {
    display.setText("Open\nPress to unlock");
    selectButton.setOnPress([]()
                            {
      if(servoMotor.close()){
        buzzer.playNotificationSound();
        isLocked = true;
      }else{
        buzzer.playError();
      } });
  }

  downButton.setOnPress([]()
                        {
                          currentMenu = Menues::timeLocker;
                          Serial.println("Down pressed!");
                          // buzzer.playNotificationSound();
                        });
}

// timer locker screen
void timeLockerScreen()
{
  display.setText("time locker");
  selectButton.setOnPress([]()
                          { currentMenu = Menues::selectTime; });

  upButton.setOnPress([]()
                      { currentMenu = Menues::opening; });

  downButton.setOnPress([]()
                        { currentMenu = Menues::calibrate; });
}

// TODO: make this more beautiful like in a class
namespace selectTimeVariables
{
  unsigned long minutesLockedOnPress;
}

namespace selectControllMassVariable
{
  unsigned long controllMassOnPressStart;
}

// select time to lock
void setTimeScreen()
{
  uint32_t hours = minutesLocked / 60;
  uint32_t minutes = minutesLocked % 60;
  String text = "lock for:\n" + String(hours) + ":" + String(minutes);
  display.setText(text);

  selectButton.setOnPress([]()
                          {
      timeSinceStartLocking = millis();
      servoMotor.close();
      currentMenu = Menues::timeLocked; });
  cancelButton.setOnPress([]()
                          { currentMenu = Menues::timeLocker; });

  downButton.setOnPress([]()
                        {
      if(minutesLocked > 1){
        minutesLocked-=1;
      } });

  downButton.setOnLongPressStart([]()
                                 {
      selectTimeVariables::minutesLockedOnPress = minutesLocked;
      Serial.println("Long press start"); });
  downButton.setOnLongPress([](double onLongPress)
                            {
      minutesLocked = selectTimeVariables::minutesLockedOnPress - onLongPress * 15;
      if(minutesLocked < 1){
        minutesLocked = 1;
      } });

  upButton.setOnPress([]()
                      {
    Serial.println(minutesLocked);
    minutesLocked+=1; });

  upButton.setOnLongPressStart([]()
                               {
                                 selectTimeVariables::minutesLockedOnPress = minutesLocked;
                                 Serial.println("Long press start");
                                 Serial.println(minutesLocked); });
  upButton.setOnLongPress([](double onLongPress)
                          { minutesLocked = selectTimeVariables::minutesLockedOnPress + onLongPress * 15; });
}

void secondsLockedScreen()
{
  unsigned long timeToGo = (minutesLocked * 60) - (millis() - timeSinceStartLocking) / 1000;

  if (timeToGo <= 0)
  {
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

void calibrateScreen()
{
  display.setText("Calibrate Weight");
  selectButton.setOnPress([]()
                          { currentMenu = Menues::tare; });

  upButton.setOnPress([]()
                      { currentMenu = Menues::timeLocker; });

  downButton.setOnPress([]()
                        {
                          // currentMenu = Menues::calibrate;
                        });
}

void tareScreen()
{
  display.setText("Tare");
  selectButton.setOnPress([]()
                          {
    loadCell.tare();
    state.setTareOffset(loadCell.getTareOffset());
    currentMenu = Menues::knownMass; });
  cancelButton.setOnPress([]()
                          { currentMenu = Menues::calibrate; });

  upButton.setOnPress([]()
                      {
                        // currentMenu = Menues::timeLocker;
                      });

  downButton.setOnPress([]()
                        {
                          // currentMenu = Menues::calibrate;
                        });
}

void enterMassScreen()
{
  String text = "calibration mass:\n" + String(controllMassInGramm) + "\ngramm";
  display.setText(text);

  selectButton.setOnPress([]()
                          {
      loadCell.calibrateWithKnowMass(controllMassInGramm);
      state.setTareFactor(loadCell.getCalFactor());
      currentMenu = Menues::calibrate; });
  cancelButton.setOnPress([]()
                          { currentMenu = Menues::tare; });

  downButton.setOnPress([]()
                        {
      if(controllMassInGramm > 1){
        controllMassInGramm-=1;
      } });

  downButton.setOnLongPressStart([]()
                                 {
      selectControllMassVariable::controllMassOnPressStart = controllMassInGramm;
      Serial.println("Long press start"); });
  downButton.setOnLongPress([](double onLongPress)
                            {
      controllMassInGramm = selectControllMassVariable::controllMassOnPressStart - onLongPress * 15;
      if(controllMassInGramm < 1){
        controllMassInGramm = 1;
      } });

  upButton.setOnPress([]()
                      { controllMassInGramm += 1; });

  upButton.setOnLongPressStart([]()
                               {
                                 selectControllMassVariable::controllMassOnPressStart = controllMassInGramm;
                                 Serial.println("Long press start"); });
  upButton.setOnLongPress([](double onLongPress)
                          { controllMassInGramm = selectControllMassVariable::controllMassOnPressStart + onLongPress * 15; });
}

void setup()
{
  Serial.begin(9600);
  // TODO: make sure the size is enough
  EEPROM.begin(512);
  display.begin();
  servoMotor.open();
  loadCell.start(500,false);
  loadCell.setTareOffset(state.getTareOffset());
  loadCell.setCalFactor(state.getTareFacotor());
  currentMenu = Menues::opening;
}

void loop()
{
  // update parts
  selectButton.update();
  cancelButton.update();
  upButton.update();
  downButton.update();
  //Serial.println(loadCell.getData());

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
  case Menues::tare:
    tareScreen();
    break;
  case Menues::knownMass:
    enterMassScreen();
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

    default:
      break;*/
  }
}