#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MLX90614.h>

#define SEALEVELPRESSURE_HPA (1013.25)

const byte ACTION_BTN_PIN = 9;

const byte TORCH_PIN = 5;
const byte SOLDERING_IRON_PIN = 10;

const byte SCREWDRIVER_FORWARD = 6;
const byte SCREWDRIVER_BACKWARDS = 7;

const byte SCREEN_WIDTH = 128;
const byte SCREEN_HEIGHT = 32;

unsigned long buttonTimer = 0;
int longPressTime = 500;

char *modes[] = {
    "Ambient",
    "Torch",
    "Screwdriver",
    "Soldering Iron",
    "IR Temp"
};

#define AMBIENT_STATS 0
#define TORCH 1
#define SCREWDRIVER 2
#define SOLDERING_IRON 3
#define IR_TEMP 4

#define N_MODES 5

byte selectedMode = 0;

boolean buttonActive = false;
boolean longPressActive = false;

//Screen
#define OLED_RESET 4
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_BME280 bme;

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

void displayText(char txt[]) {
    int x1, y1, w, h;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.getTextBounds(txt, 0, SCREEN_HEIGHT/2, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH/2) - w / 2, SCREEN_HEIGHT/2);
    display.print(txt);
    display.display();
}

void displayText(__FlashStringHelper * txt) {
    int x1, y1, w, h;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.getTextBounds(txt, 0, SCREEN_HEIGHT/2, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH/2) - w / 2, SCREEN_HEIGHT/2);
    display.print(txt);
    display.display();
}

void displayTextRaw(char txt[]) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print(txt);
    display.display();
}

int readActionButton() {
    return digitalRead(ACTION_BTN_PIN);
}

void buttonCheck(void (*shortPress)(void), void (*longPress)(void)) {
    if (readActionButton() == LOW) {
		if (!buttonActive) {
			buttonActive = true;
			buttonTimer = millis();
		}
		if ((millis() - buttonTimer > longPressTime) && (!longPressActive)) {
			longPressActive = true;
			longPress();
		}
	} else {
		if (buttonActive) {
			if (longPressActive) {
				longPressActive = false;
			} else {
				shortPress();
			}
			buttonActive = false;
		}
	}
}

void advanceMode() {
    if (selectedMode == (N_MODES - 1)) {
        selectedMode = 0;
    } else {
        selectedMode++;
    }
    displayText(modes[selectedMode]);
}

void torch() {
    displayText(F("Torch on"));
    digitalWrite(TORCH_PIN, HIGH);
    delay(600);
    while (readActionButton() != LOW){};
    digitalWrite(TORCH_PIN, LOW);
}

void ambientSensor() {
    displayText(F("Starting sensor..."));
    delay(600);
    while (readActionButton() != LOW) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print(F("Temperature: "));
        display.print(bme.readTemperature(), 2);
        display.println(F("C"));
        display.print(F("Pressure: "));
        display.print(bme.readPressure(), 0);
        display.println(F("hPa"));
        display.print(F("Altitude: "));
        display.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
        display.println(F("m"));
        display.print("Humidity: ");
        display.println(bme.readHumidity(), 2);
        display.println(F("%"));
        display.display();
        delay(700);
    };
}

void solderingIronMode() {
    digitalWrite(SOLDERING_IRON_PIN, HIGH);
    displayText(F("Iron heating..."));
    delay(1000);
    while (readActionButton() != LOW) {};
    digitalWrite(SOLDERING_IRON_PIN, LOW);
}

void screwDriverMode() {
    display.clearDisplay();
    display.drawChar(2, SCREEN_HEIGHT/2, 0x18, SSD1306_WHITE, SSD1306_BLACK, 1, 1);
    display.setCursor(4, SCREEN_HEIGHT/2);
    display.print(F(" Screwdriver Active"));
    display.display();
    byte direction = SCREWDRIVER_FORWARD;
    bool wasDown = false;
    bool hasSent = true;
    bool hasCountedClicks = false;
    long startHold = 0;
    long endTime = 0;
    byte clicks = 0;
    while (true)
    {
        if (readActionButton() == LOW) {
            if (!wasDown) {
                startHold = millis();
                wasDown = true;
                if ((millis() - endTime) < 400) {
                    clicks++;
                }
            } else if ((millis() - startHold) > 1000) {
                if (direction == SCREWDRIVER_FORWARD) {
                    digitalWrite(SCREWDRIVER_FORWARD, HIGH);
                    digitalWrite(SCREWDRIVER_BACKWARDS, LOW);
                } else {
                    digitalWrite(SCREWDRIVER_FORWARD, LOW);
                    digitalWrite(SCREWDRIVER_BACKWARDS, HIGH);
                }
                while(readActionButton() == LOW) {}
                digitalWrite(SCREWDRIVER_FORWARD, LOW);
                digitalWrite(SCREWDRIVER_BACKWARDS, LOW);
            }
            hasSent = false;
            hasCountedClicks = false;
        } else {
            if (!hasSent) {
                endTime = millis();
                delay(100);
            }
            hasSent = true;
            wasDown = false;
            if ((millis() - endTime) > 600 && !hasCountedClicks) {
                hasCountedClicks = true;
                clicks++;
                if (clicks == 2) {
                    display.clearDisplay();
                    if (direction == SCREWDRIVER_FORWARD) {
                        direction = SCREWDRIVER_BACKWARDS;
                        display.drawChar(2, SCREEN_HEIGHT/2, 0x18, SSD1306_WHITE, SSD1306_BLACK, 1, 1);
                    } else {
                        direction = SCREWDRIVER_FORWARD;
                        display.drawChar(2, SCREEN_HEIGHT/2, 0x19, SSD1306_WHITE, SSD1306_BLACK, 1, 1);
                    }
                    display.setCursor(4, SCREEN_HEIGHT/2);
                    display.print(" Screwdriver Active");
                    display.display();
                } else if (clicks == 3) {
                    break;
                }
                clicks = 0;
            }
        }
    }
}

void irTemp() {
    displayText(F("IR Starting.."));
    delay(300);
    while(readActionButton() != LOW) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print(F("\n\nTemp: "));
        display.print(mlx.readObjectTempC(), 2);
        display.display();
        delay(600);
    }
}

void select() {
    switch (selectedMode)
    {
    case TORCH:
        torch();
        break;
    case AMBIENT_STATS:
        ambientSensor();
        break;
    case SOLDERING_IRON:
        solderingIronMode();
        break;
    case SCREWDRIVER:
        screwDriverMode();
        break;
    case IR_TEMP:
        irTemp();
        break;
    default:
        break;
    }
    displayText(modes[selectedMode]);
}

void setup() {
    Serial.begin(115200);
    //Action button of the screwdriver
    pinMode(ACTION_BTN_PIN, INPUT_PULLUP);
    pinMode(TORCH_PIN, OUTPUT);
    digitalWrite(TORCH_PIN, LOW);
    pinMode(SOLDERING_IRON_PIN, OUTPUT);
    digitalWrite(SOLDERING_IRON_PIN, LOW);
    pinMode(SCREWDRIVER_FORWARD, OUTPUT);
    digitalWrite(SCREWDRIVER_FORWARD, LOW);
    pinMode(SCREWDRIVER_BACKWARDS, OUTPUT);
    digitalWrite(SCREWDRIVER_BACKWARDS, LOW);
    //Screen initialization
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS) || !bme.begin(0x76)) {
        Serial.println("Sensor or screen initialization failed");
        while(1);
    }

    mlx.begin();
    displayText(modes[selectedMode]);
}

void loop() {
    //Button detection
    buttonCheck(advanceMode, select);
}