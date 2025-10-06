// Adafrut SSD1306 Disoplay class that provides a set headline and set subline methods
#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class Display {
private:
    Adafruit_SSD1306 display;
    String headline;
    String subline;

    int width;
    int height;
public:
    Display(int width, int height, int resetPin) : display(width, height, &Wire, resetPin), width(width), height(height) {
        headline = "";
        subline = "";
    }

    void begin() {
        display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
    }

    void setHeadline(String text) {
        headline = text;
        updateDisplay();
    }

    void setSubline(String text) {
        subline = text;
        updateDisplay();
    }

    void updateDisplay() {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(2);
        display.print(headline + "\n" + subline);
        /*display.setCursor(0, height/2);
        display.setTextSize(1);
        display.print(subline);*/
        display.display();
    }
};