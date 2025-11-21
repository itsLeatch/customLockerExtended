// Adafrut SSD1306 Disoplay class that provides a set headline and set subline methods
#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class Display {
private:
    Adafruit_SSD1306 display;
    String text = "";

    int width;
    int height;
public:
    Display(int width, int height, int resetPin) : display(width, height, &Wire, resetPin), width(width), height(height) {
        
    }

    void begin() {
        display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
    }

    void updateDisplay() {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(2);
        display.print(text);
        /*display.setCursor(0, height/2);
        display.setTextSize(1);
        display.print(subline);*/
        display.display();
    }

    void setText(const String& inputText) {
        text = inputText;
        updateDisplay();
    }


};