#include <BleMouse.h>
#include "EspUsbHost.h"


#define SHOW_REAL_BATTERY
#define DISABLE_USB


BleMouse bleMouse("Ball Mouse");

#ifdef SHOW_REAL_BATTERY
  #define BATTERY_UPDATE_INTERVAL 5000  // например, 30000 мс = 30 секунд
  #define BATTERY_ADC_PIN 6
  #define BATTERY_VOLTAGE_DIVIDER_RATIO 2.0f
  #define BATTERY_VOLTAGE_MAX 5.0f
  #define BATTERY_VOLTAGE_MIN 3.5f
  #include "BatteryMonitor.h"
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
  Serial.begin(115200);
  delay(500);


  bleMouse.begin();
  Serial.println("BLE Mouse started");

#ifndef DISABLE_USB
  usbHost.begin();
  Serial.println("USB Host started");
#endif //DISABLE_USB
}

void loop() {
#ifndef DISABLE_USB
  usbHost.task();
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
