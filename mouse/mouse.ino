#include <BleMouse.h>
#include "EspUsbHost.h"
#include <LedControl.h>
#include "BatteryMonitor.h"

#define LED_PIN     21   // GPIO пин светодиода (скорее всего 48)
#define NUM_LEDS     1   // Один RGB светодиод


// #define DISABLE_RGB_UUID   // отключить управление диодои по BLE
// #define DISABLE_USB        // отключить чтение реальной usb мыши

#define SHOW_REAL_BATTERY     // считывать реальный уровень батареи
#define RGB_LED               // включить подстветку

BleMouse bleMouse("Ball Mouse");

#ifdef RGB_LED
Adafruit_NeoPixel rgbLed(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
LedControl ledControl(rgbLed);
#endif //RGB_LED

#ifdef SHOW_REAL_BATTERY
// Параметры для считывания уровня батареи
  #define BATTERY_UPDATE_INTERVAL 5000  // например, 30000 мс = 30 секунд
  #define BATTERY_ADC_PIN 6
  #define BATTERY_VOLTAGE_DIVIDER_RATIO 2.0f
  #define BATTERY_VOLTAGE_MAX 5.0f
  #define BATTERY_VOLTAGE_MIN 3.5f
  BatteryMonitor battery(BATTERY_ADC_PIN, BATTERY_VOLTAGE_DIVIDER_RATIO, BATTERY_VOLTAGE_MAX, BATTERY_VOLTAGE_MIN);
#endif//SHOW_REAL_BATTERY

#ifndef DISABLE_USB
// Расширяем класс для обработки USB мыши
class MyEspUsbHost : public EspUsbHost {
  void onMouseButtons(hid_mouse_report_t report, uint8_t last_buttons) {
    if (!bleMouse.isConnected()) return;

    // Нажатия и отпускания кнопок
    if ((last_buttons & MOUSE_BUTTON_LEFT) != (report.buttons & MOUSE_BUTTON_LEFT)) {
      if (report.buttons & MOUSE_BUTTON_LEFT) bleMouse.press(MOUSE_LEFT);
      else bleMouse.release(MOUSE_LEFT);
    }
    if ((last_buttons & MOUSE_BUTTON_RIGHT) != (report.buttons & MOUSE_BUTTON_RIGHT)) {
      if (report.buttons & MOUSE_BUTTON_RIGHT) bleMouse.press(MOUSE_RIGHT);
      else bleMouse.release(MOUSE_RIGHT);
    }
    if ((last_buttons & MOUSE_BUTTON_MIDDLE) != (report.buttons & MOUSE_BUTTON_MIDDLE)) {
      if (report.buttons & MOUSE_BUTTON_MIDDLE) bleMouse.press(MOUSE_MIDDLE);
      else bleMouse.release(MOUSE_MIDDLE);
    }
    // Дополнительные кнопки (если поддерживаются)
  }

  void onMouseMove(hid_mouse_report_t report) {
    if (!bleMouse.isConnected()) return;

    // Передаём перемещения и скролл через BLE
    bleMouse.move(report.x, report.y, report.wheel);
  }
};
MyEspUsbHost usbHost;
#endif //DISABLE_USB

void setup() {
  // Serial.begin(115200);
  //-----------Запуск мыши — зелёный--------------
#ifdef RGB_LED
  ledControl.begin();
  ledControl.forceColor(0, 255, 0); // Зеленый
#endif//RGB_LED
  delay(500);

  //--------Инициализация BLE - красный-----------
#ifdef RGB_LED
  ledControl.forceColor(255, 0, 0); // Красный
#endif//RGB_LED
  // Запуск BLE мыши (HID)
  bleMouse.init();
#ifdef RGB_LED
#ifndef DISABLE_RGB_UUID
  // Создаём BLE-сервис для подстветки
  ledControl.setupBLEService(bleMouse.server);
#endif//DISABLE_RGB_UUID
#endif//RGB_LED
  // Запуск BLE мыши (HID)
  bleMouse.begin();
  // BLE готов — красный

  //---------Инициализация USB — синий------------
#ifdef RGB_LED
  ledControl.forceColor(0, 0, 255);   // Синий
#endif//RGB_LED
#ifndef DISABLE_USB
  usbHost.begin();
#endif

  //---Мышь готова - загружаем сохранённый цвет---
#ifdef RGB_LED
  ledControl.applyColor();
#endif//RGB_LED
}

void loop() {
#ifndef DISABLE_USB
  // чтение мыши
  usbHost.task();
#else
  // симуляция движения мыши
  if (bleMouse.isConnected())
  {
    bleMouse.move(5, 0, 0);
    delay(1000);
  }
#endif //DISABLE_USB

#ifdef SHOW_REAL_BATTERY
  // чтение уровня батареи
  static unsigned long lastBatteryUpdate = 0;
  if (bleMouse.isConnected()) {
    unsigned long now = millis();
    if (now - lastBatteryUpdate > BATTERY_UPDATE_INTERVAL) {
      lastBatteryUpdate = now;

      uint8_t batteryLevel = battery.readLevelPercent();
      Serial.printf("Battery Level: %d%%\n", batteryLevel);

      bleMouse.setBatteryLevel(batteryLevel);
    }
  }
#endif
}