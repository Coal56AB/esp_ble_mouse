#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Adafruit_NeoPixel.h>
#include <Preferences.h>
#include <BLECharacteristic.h>
#include <BLEServer.h>
#include <BLEDevice.h>
#include <BleMouse.h>

#define LED_NAMESPACE "led"
#define COLOR_SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define COLOR_CHAR_R_UUID         "12345678-1234-5678-1234-56789abcdef1"
#define COLOR_CHAR_G_UUID         "12345678-1234-5678-1234-56789abcdef2"
#define COLOR_CHAR_B_UUID         "12345678-1234-5678-1234-56789abcdef3"
#define COLOR_CHAR_BRIGHTNESS_UUID "12345678-1234-5678-1234-56789abcdef4"

#define RED_DEFAULT     255
#define GREEN_DEFAULT   0
#define BLUE_DEFAULT    255
#define BRIGHT_DEFAULT  50

class LedControl {
public:
  LedControl(Adafruit_NeoPixel &led)
    : rgbLed(led) {}

  void begin();
  void setupBLEService(BLEServer *server);
  void applyColor();
  void forceColor(uint8_t r, uint8_t g, uint8_t b);


#ifndef RED_DEFAULT
#define RED_DEFAULT 0
#endif

#ifndef GREEN_DEFAULT
#define GREEN_DEFAULT 0
#endif

#ifndef BLUE_DEFAULT
#define BLUE_DEFAULT 0
#endif

#ifndef BRIGHT_DEFAULT
#define BRIGHT_DEFAULT 0
#endif
private:
  Adafruit_NeoPixel &rgbLed;
  Preferences preferences;

  int currentR = RED_DEFAULT, currentG = GREEN_DEFAULT, currentB = BLUE_DEFAULT, currentBrightness = BRIGHT_DEFAULT;
  BLECharacteristic *colorCharacteristic = nullptr;

  void loadSettings();
  void saveSettings();
  uint8_t correctedBrightness(uint8_t percent);

class ColorCallbacks : public BLECharacteristicCallbacks {
public:
  ColorCallbacks(LedControl *parent, uint8_t paramId)
    : _parent(parent), _paramId(paramId) {}

  void onWrite(BLECharacteristic *characteristic) override {
    String value = characteristic->getValue();
    if (value.length() >= 1) {
      uint8_t val = static_cast<uint8_t>(value[0]);
      switch (_paramId) {
        case 0: _parent->currentR = val; break;
        case 1: _parent->currentG = val; break;
        case 2: _parent->currentB = val; break;
        case 3: _parent->currentBrightness = val; break;
      }
      characteristic->setValue(&val, 1);
      _parent->applyColor();
      _parent->saveSettings();

      Serial.printf("Set param %d = %d\n", _paramId, val);
    }
  }

  void onRead(BLECharacteristic *characteristic) override {
    uint8_t val = 0;
    switch (_paramId) {
      case 0: val = _parent->currentR; break;
      case 1: val = _parent->currentG; break;
      case 2: val = _parent->currentB; break;
      case 3: val = _parent->currentBrightness; break;
    }
    characteristic->setValue(&val, 1);  // Обновляем значение характеристики перед чтением

    Serial.printf("Read param %d value: %d\n", _paramId, val);
  }

private:
  LedControl *_parent;
  uint8_t _paramId;
};

};
#endif
