
#include <Arduino.h>
#include <EEPROM.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

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

const std::string SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const std::string CURRENT_CALORIES_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
const std::string MIN_CALORIES_TO_OPEN_UUID = "33aedfe5-62a7-4f57-aad2-00d409fcd44c";

BLECharacteristic *pCurrentCalories = nullptr;
BLECharacteristic *pMinCaloriesToOpen = nullptr;

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
  takeOutRewardingSweets,
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
  float caloriesOnStart;
  float weightOnClose;

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
    // update the characteristic
    pMinCaloriesToOpen->setValue(String(minCaloriesToOpen).c_str());
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

  void setCaloriesOnStart(const float &newCaloriesOnStart)
  {
    caloriesOnStart = newCaloriesOnStart;
    save();
  }
  // TODO: fetch this data from the app
  float getCaloriesOnStart()
  {
    readValues();

    if (!(caloriesOnStart >= 0.0))
    {
      caloriesOnStart = 0.0;
    }
    return caloriesOnStart;
  }

  void setWeightOnClose(const float &newWeightOnClose)
  {
    weightOnClose = newWeightOnClose;
    save();
  }

  float getWeightOnClose()
  {
    readValues();
    return weightOnClose;
  }

} state;

float getBurnedCalories()
{
  std::string data = pCurrentCalories->getValue().c_str();
  float dataAsFloat = 0.0f;
  try
  {
    dataAsFloat = std::stof(data);
  }
  catch (...)
  {
    dataAsFloat = 0.0f;
  }
  return dataAsFloat;
}

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

  selectButton.setOnPress([]()
                          { currentMenu = Menues::inputMinCaloriesToOpen; });
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
  selectButton.setOnPress([]()
                          {
    state.setCaloriesOnStart(getBurnedCalories());
    state.setWeightOnClose(loadCell.getData());
    buzzer.playNotificationSound();
      servoMotor.close();
currentMenu = Menues::caloriesToGo; });

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
  float currentCalories = state.getMinCaloriesToOpen() - (getBurnedCalories() - state.getCaloriesOnStart());
  String text = "calories to go:\n" + String(currentCalories);
  display.setText(text);

  if (currentCalories <= 0)
  {
    servoMotor.open();
    buzzer.playRewardSound();
    currentMenu = Menues::takeOutRewardingSweets;
    state.setWeightOnClose(loadCell.getData()); // TODO: removve this because it is twice
  }

  selectButton.setOnPress([]() {});
  // TODO: remove this function since the screen should disappear when the calories goal is reached
  cancelButton.setOnPress([]()
                          { currentMenu = Menues::caloriesLocker; });
}

void takeOutRewardingSweets()
{
  float currentWeight = loadCell.getData();
  float earnedLeft = (getBurnedCalories() /*- state.getCaloriesOnStart()*/) - ((state.getWeightOnClose() - currentWeight) / 100 * state.getKcalPerOnehundredGramm());

  String text = "Left KCal:\n" + String(earnedLeft);
  display.setText(text);
  Serial.println("burned: " + String(getBurnedCalories()));
  Serial.println("Ratio: " + String(((state.getWeightOnClose() - currentWeight) / 100 * state.getKcalPerOnehundredGramm())));
  Serial.println("onclose: " + String(state.getWeightOnClose()));
  if (earnedLeft <= 0)
  {
    buzzer.playError();
  }
  if (earnedLeft > 0)
  {

    selectButton.setOnPress([]()
                            { currentMenu = Menues::inputCaloriesToWeightRatio; });

    cancelButton.setOnPress([]()
                            { currentMenu = Menues::caloriesLocker; });
  }
  else
  {
    selectButton.setOnPress([]() {});
    cancelButton.setOnPress([]() {});
  }
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

BLEServer *pServer = nullptr;
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

  // init bluetooth
  BLEDevice::init("smart locker");
  pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  // current Calories
  pCurrentCalories = pService->createCharacteristic(
      CURRENT_CALORIES_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);
  pCurrentCalories->setValue("1.0");
  // min calories to open
  // TODO: notice, that there should be also write functionality in the future
  pMinCaloriesToOpen = pService->createCharacteristic(
      MIN_CALORIES_TO_OPEN_UUID,
      BLECharacteristic::PROPERTY_READ);
  pMinCaloriesToOpen->setValue(String(state.getMinCaloriesToOpen()).c_str());

  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}

// clears all button bindings to prevent button functions from previous screens interfering with the current screen
void clearButtonBindings()
{
  selectButton.setOnPress([]() {});
  selectButton.setOnLongPressStart([]() {});
  selectButton.setOnLongPress([](double timeSincePress) {});

  cancelButton.setOnPress([]() {});
  cancelButton.setOnLongPressStart([]() {});
  cancelButton.setOnLongPress([](double timeSincePress) {});

  upButton.setOnPress([]() {});
  upButton.setOnLongPressStart([]() {});
  upButton.setOnLongPress([](double timeSincePress) {});

  downButton.setOnPress([]() {});
  downButton.setOnLongPressStart([]() {});
  downButton.setOnLongPress([](double timeSincePress) {});
}

void loop()
{
  // check if bloetooth is connected othervise start advertising again
  if (pServer->getConnectedCount() == 0)
  {
  BLEDevice::startAdvertising();
  }

  // update parts
  selectButton.update();
  cancelButton.update();
  upButton.update();
  downButton.update();

  // clear all button bindings to prevent interference between screens
  clearButtonBindings();

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
  case Menues::takeOutRewardingSweets:
    takeOutRewardingSweets();
    break;

    /*default:
      break;*/
  }
}