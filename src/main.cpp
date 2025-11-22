
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
  caloriesLocker,
  inputCaloriesToWeightRatio,
  inputMinCaloriesToOpen,
  caloriesToGo,
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
  float calibrationFactor;
  long tareOffset;

  float kcalPerOnehundredGramm;
  float minCaloriesToOpen;
  float alreadyBurnedCalories;

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
  void setCalibrationFactor(const float &newCalibrationFactor)
  {
    calibrationFactor = newCalibrationFactor;
    save();
  }

  float getCalibrationFacotor()
  {
    readValues();
    // make sure the tare factor is not nan or 0
    if (!(calibrationFactor > 0.0))
    {
      calibrationFactor = 1;
    }

    return calibrationFactor;
  }

  void setTareOffset(const long &newTareOffset)
  {
    tareOffset = newTareOffset;
    save();
  }

  long getTareOffset()
  {
    readValues();

    if (!(tareOffset >= 0))
    {
      tareOffset = 0;
    }
    return tareOffset;
  }

  void setKcalPerOnehundredGramm(const float &newKcalPerOnehundredGramm)
  {
    kcalPerOnehundredGramm = newKcalPerOnehundredGramm;
    save();
  }

  float getKcalPerOnehundredGramm()
  {
    readValues();

    if (!(kcalPerOnehundredGramm > 0.0))
    {
      kcalPerOnehundredGramm = 100.0;
    }
    return kcalPerOnehundredGramm;
  }

  void setMinCaloriesToOpen(const float &newMinCaloriesToOpen)
  {
    minCaloriesToOpen = newMinCaloriesToOpen;
    save();
  }

  float getMinCaloriesToOpen()
  {
    readValues();

    if (!(minCaloriesToOpen > 0.0))
    {
      minCaloriesToOpen = 500.0;
    }
    return minCaloriesToOpen;
  }

  void setAlreadyBurnedCalories(const float &newAlreadyBurnedCalories)
  {
    alreadyBurnedCalories = newAlreadyBurnedCalories;
    save();
  }
  // TODO: fetch this data from the app
  float getAlreadyBurnedCalories()
  {
    readValues();

    if (!(alreadyBurnedCalories >= 0.0))
    {
      alreadyBurnedCalories = 0.0;
    }
    return alreadyBurnedCalories;
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
                        { currentMenu = Menues::caloriesLocker; });
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

namespace selectCalorieWeightRatio
{
  float calorieWeightRatioOnPressStart;
}

namespace selectMinCaloriesToOpen
{
  unsigned minCaloriesToOpenOnPressStart;
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

void lockByCaloriesScreen()
{
  display.setText("lock by\nCalories");
  selectButton.setOnPress([]()
                          { currentMenu = Menues::inputCaloriesToWeightRatio; });

  upButton.setOnPress([]()
                      { currentMenu = Menues::timeLocker; });

  downButton.setOnPress([]()
                        { currentMenu = Menues::calibrate; });
}

void inputCaloriesToWeightRatioScreen()
{
  String text = "Calories per 100g:\n" + String(state.getKcalPerOnehundredGramm());
  display.setText(text);

    selectButton.setOnPress([]() {
      currentMenu = Menues::inputMinCaloriesToOpen;
    });
  cancelButton.setOnPress([]()
                          { currentMenu = Menues::caloriesLocker; });

  downButton.setOnPress([]()
                        {
      if(state.getKcalPerOnehundredGramm() > 1){
        state.setKcalPerOnehundredGramm(state.getKcalPerOnehundredGramm() - 1);
      } });

  downButton.setOnLongPressStart([]()
                                 { selectCalorieWeightRatio::calorieWeightRatioOnPressStart = state.getKcalPerOnehundredGramm(); });
  downButton.setOnLongPress([](double onLongPress)
                            {
      state.setKcalPerOnehundredGramm(selectCalorieWeightRatio::calorieWeightRatioOnPressStart - onLongPress * 15);
      if(state.getKcalPerOnehundredGramm() < 1){
        state.setKcalPerOnehundredGramm(1);
      } });

  upButton.setOnPress([]()
                      { state.setKcalPerOnehundredGramm(state.getKcalPerOnehundredGramm() + 1); });

  upButton.setOnLongPressStart([]()
                               { selectCalorieWeightRatio::calorieWeightRatioOnPressStart = state.getKcalPerOnehundredGramm(); });
  upButton.setOnLongPress([](double onLongPress)
                          { state.setKcalPerOnehundredGramm(selectCalorieWeightRatio::calorieWeightRatioOnPressStart + onLongPress * 15); });
}

void inputCaloriesToOpenScreen()
{
  String text = "min Calories to open" + String(state.getMinCaloriesToOpen());
  display.setText(text);
  selectButton.setOnPress([]() {
currentMenu = Menues::caloriesToGo;
  });

  cancelButton.setOnPress([]()
                          { currentMenu = Menues::inputCaloriesToWeightRatio; });

  downButton.setOnPress([]()
                        {
      if(state.getMinCaloriesToOpen() > 1){
        state.setMinCaloriesToOpen(state.getMinCaloriesToOpen() - 1);
      } });
  downButton.setOnLongPressStart([]()
                                 { selectMinCaloriesToOpen::minCaloriesToOpenOnPressStart = state.getMinCaloriesToOpen(); });
  downButton.setOnLongPress([](double onLongPress)
                            {
      state.setMinCaloriesToOpen(selectMinCaloriesToOpen::minCaloriesToOpenOnPressStart - onLongPress * 15);
      if(state.getMinCaloriesToOpen() < 1){
        state.setMinCaloriesToOpen(1);
      } });
  upButton.setOnPress([]()
                      { state.setMinCaloriesToOpen(state.getMinCaloriesToOpen() + 1); });
  upButton.setOnLongPressStart([]()
                               { selectMinCaloriesToOpen::minCaloriesToOpenOnPressStart = state.getMinCaloriesToOpen(); });
  upButton.setOnLongPress([](double onLongPress)
                          { state.setMinCaloriesToOpen(selectMinCaloriesToOpen::minCaloriesToOpenOnPressStart + onLongPress * 15); });
}

void caloriesToGoScreen()
{
  float currentWeight = loadCell.getData();
  float currentCalories = (currentWeight / 100.0) * state.getKcalPerOnehundredGramm();
  //float caloriesToGo = state.getMinCaloriesToOpen() - (state.getAlreadyBurnedCalories() + currentCalories);
  //TODO: make this interactive according to the burned calories from the app
  String text = "calories to go:\n" + String(currentCalories);
  display.setText(text);

  selectButton.setOnPress([]()
                          {});
                          //TODO: remove this function since the screen should disappear when the calories goal is reached
  cancelButton.setOnPress([]()
                          { currentMenu = Menues::caloriesLocker; });
}

void calibrateScreen()
{
  display.setText("Calibrate Weight");
  selectButton.setOnPress([]()
                          { currentMenu = Menues::tare; });

  upButton.setOnPress([]()
                      { currentMenu = Menues::caloriesLocker; });

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
      state.setCalibrationFactor(loadCell.getCalFactor());
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
  loadCell.start(500, false);
  loadCell.setTareOffset(state.getTareOffset());
  loadCell.setCalFactor(state.getCalibrationFacotor());
  currentMenu = Menues::opening;
}

void loop()
{
  // update parts
  selectButton.update();
  cancelButton.update();
  upButton.update();
  downButton.update();
  // Serial.println(loadCell.getData());

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
  case Menues::caloriesLocker:
    lockByCaloriesScreen();
    break;

  case Menues::inputCaloriesToWeightRatio:
    inputCaloriesToWeightRatioScreen();
    break;

    case Menues::inputMinCaloriesToOpen:
      inputCaloriesToOpenScreen();
      break;

    case Menues::caloriesToGo:
      caloriesToGoScreen();
      break;

    /*default:
      break;*/
  }
}