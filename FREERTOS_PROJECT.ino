#define BLYNK_TEMPLATE_ID "TMPL65jsxvo5J"
#define BLYNK_TEMPLATE_NAME "GAMEPROJECT"
#define BLYNK_AUTH_TOKEN "aDETZRfWY2gLy48GMwlZmQP2Yh_cvzi4"
#define WIFI_SSID "Leena"
#define WIFI_PASSWORD "affouri0599"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

const int blueLed = 14;
const int orangeLed = 26;
const int yellowLed = 27;
const int redLed = 32;
const int ledPins[] = {blueLed, orangeLed, yellowLed, redLed};

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define VIRTUAL_START_BUTTON V0
#define VIRTUAL_BLUE_BUTTON V1
#define VIRTUAL_ORANGE_BUTTON V2
#define VIRTUAL_YELLOW_BUTTON V3
#define VIRTUAL_RED_BUTTON V4

TaskHandle_t GameTaskHandle;
QueueHandle_t buttonQueue;

volatile int currentLevel = 1;
const int maxLevel = 20;
volatile int sequenceLength;
volatile int gameSequence[22];
volatile int playerSequence[22];
volatile int sequenceIndex = 0;
volatile bool isGameActive = false;
volatile int trialCount = 0;
const int maxTrials = 3;
volatile int playerScore = 0;

void turnOffAllLeds() {
    for (int i = 0; i < 4; i++) {
        digitalWrite(ledPins[i], LOW);
    }
}

void displaySequence() {
    lcd.clear();
    lcd.print("WATCH closely!");
    for (int i = 0; i < sequenceLength; i++) {
        int ledIndex = random(0, 4);
        gameSequence[i] = ledIndex;

        digitalWrite(ledPins[ledIndex], HIGH);
        vTaskDelay(pdMS_TO_TICKS(1000));
        digitalWrite(ledPins[ledIndex], LOW);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    lcd.clear();
    lcd.print("REPEAT Sequence");
    sequenceIndex = 0;
}

void gameTask(void *pvParameters) {
    while (1) {
        if (isGameActive) {
            displaySequence();
            int index = 0;
            int ledIndex;
            int mistakes = 0;

            while (index < sequenceLength) {
                if (xQueueReceive(buttonQueue, &ledIndex, portMAX_DELAY) == pdPASS) {
                    playerSequence[index] = ledIndex;
                    digitalWrite(ledPins[ledIndex], HIGH);
                    vTaskDelay(pdMS_TO_TICKS(200));
                    digitalWrite(ledPins[ledIndex], LOW);
                    if (ledIndex != gameSequence[index]) {
                        mistakes++;
                    }
                    index++;
                }
            }

            if (mistakes == 0) {
                playerScore += sequenceLength;
                lcd.clear();
                lcd.print("Level " + String(currentLevel) + " Complete");
                lcd.setCursor(0, 1);
                lcd.print("Score: " + String(playerScore));
                vTaskDelay(pdMS_TO_TICKS(2000)); // Display score for 2 seconds

                if (currentLevel == maxLevel) {
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("You WIN!");
                    lcd.setCursor(0, 1);
                    lcd.print("Final Score: " + String(playerScore));
                    vTaskDelay(pdMS_TO_TICKS(4000)); // Show final win message
                    lcd.clear();
                    lcd.print("TO PLAY NEW GAME");
                    lcd.setCursor(0, 1);
                    lcd.print("PRESS START");
                    isGameActive = false;
                } else {
                    currentLevel++;
                    sequenceLength = currentLevel + 2;
                    trialCount = 0;
                }
            } else {
                trialCount++;
                if (trialCount >= maxTrials) {
                    lcd.clear();
                    lcd.print("You LOST,");
                    lcd.setCursor(0, 1);
                    lcd.print("REPLAY AGAIN!");
                    isGameActive = false;
                } else {
                    lcd.clear();
                    lcd.print("Try Again");
                    lcd.setCursor(0, 1);
                    lcd.print("Trials Left: " + String(maxTrials - trialCount));
                    vTaskDelay(pdMS_TO_TICKS(2000));
                }
            }

            if (!isGameActive) {
                currentLevel = 1;  // Reset level for new game
                sequenceLength = currentLevel + 2;
                trialCount = 0;
                playerScore = 0;  // Reset score after displaying
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

void setup() {
    Serial.begin(115200);
    for (int i = 0; i < 4; i++) {
        pinMode(ledPins[i], OUTPUT);
    }
    turnOffAllLeds();

    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Press Start");
    lcd.setCursor(0, 1);
    lcd.print("to BEGIN Game");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);

    buttonQueue = xQueueCreate(10, sizeof(int));

    xTaskCreate(gameTask, "Game Task", 2048, NULL, 2, &GameTaskHandle);
}

void loop() {
    Blynk.run();
}

BLYNK_WRITE(VIRTUAL_START_BUTTON) {
    int buttonState = param.asInt();
    if (buttonState == 1 && !isGameActive) {
        isGameActive = true;
        currentLevel = 1;  // Reset level
        sequenceLength = currentLevel + 2;  // Reset sequence length
        trialCount = 0;  // Reset trials
        playerScore = 0;  // Reset score
        xTaskNotifyGive(GameTaskHandle);
    }
}

void handleButtonPress(int ledIndex) {
    xQueueSendToBack(buttonQueue, &ledIndex, 0);
}

BLYNK_WRITE(VIRTUAL_BLUE_BUTTON) { if (param.asInt()) handleButtonPress(0); }
BLYNK_WRITE(VIRTUAL_ORANGE_BUTTON) { if (param.asInt()) handleButtonPress(1); }
BLYNK_WRITE(VIRTUAL_YELLOW_BUTTON) { if (param.asInt()) handleButtonPress(2); }
BLYNK_WRITE(VIRTUAL_RED_BUTTON) { if (param.asInt()) handleButtonPress(3); }
