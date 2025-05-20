#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <Arduino.h>

class BatteryMonitor {
private:
  uint8_t _adcPin;
  const float _vref = 3.3;
  const int _adcMax = 4095;
  const float _voltageDividerRatio;
  const float _vMax;
  const float _vMin;

public:
  BatteryMonitor(uint8_t adcPin,
                 float voltageDividerRatio = 2.0,
                 float vMax = 4.2,
                 float vMin = 3.0) :
    _adcPin(adcPin),
    _voltageDividerRatio(voltageDividerRatio),
    _vMax(vMax),
    _vMin(vMin)
  {
    pinMode(_adcPin, INPUT);
  }

  uint8_t readLevelPercent() {
    int raw = analogRead(_adcPin);
    float voltage = (raw * (_vref / _adcMax)) * _voltageDividerRatio;

    if (voltage >= _vMax) return 100;
    if (voltage <= _vMin) return 0;

    return (uint8_t)(100 * (voltage - _vMin) / (_vMax - _vMin));
  }
};

#endif // BATTERY_MONITOR_H
