#include "LedControl.h"
#include <cmath>

void LedControl::begin() {
    rgbLed.begin();
    loadSettings();
    applyColor();
}

// Вспомогательная функция для создания дескриптора с описанием
BLEDescriptor* createUserDescription(const char* description) {
    BLEDescriptor* desc = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
    desc->setValue(description);
    return desc;
}

// Вспомогательная функция для создания дескриптора формата uint8
BLEDescriptor* createPresentationFormatDescriptor() {
    // Формат Presentation Format Descriptor (0x2904)
    // Format: uint8 (0x04), Exponent: 0, Unit: unitless (0x2700), Namespace: Bluetooth SIG (1), Description: 0
    uint8_t presentationFormat[7] = {
        0x04,       // Format: uint8
        0x00,       // Exponent
        0x00, 0x27, // Unit (unitless) - обратите внимание на порядок байт: младший сначала
        0x01,       // Namespace (Bluetooth SIG)
        0x00, 0x00  // Description
    };
    BLEDescriptor* desc = new BLEDescriptor(BLEUUID((uint16_t)0x2904));
    desc->setValue(presentationFormat, sizeof(presentationFormat));
    return desc;
}
void LedControl::setupBLEService(BLEServer *server) {
  if(server == NULL)
  {
    return;
  }

    BLEService *colorService = server->createService(COLOR_SERVICE_UUID);

    // --- Красный ---
    BLECharacteristic *rChar = colorService->createCharacteristic(
        COLOR_CHAR_R_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );
    // rChar->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE);
    rChar->setValue(currentR);
    rChar->setCallbacks(new ColorCallbacks(this, 0));
    // rChar->addDescriptor(createUserDescription("Red"));

    // --- Зелёный ---
    BLECharacteristic *gChar = colorService->createCharacteristic(
        COLOR_CHAR_G_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );
    // gChar->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE);
    gChar->setValue(currentG);
    gChar->setCallbacks(new ColorCallbacks(this, 1));
    // gChar->addDescriptor(createUserDescription("Green"));

    // --- Синий ---
    BLECharacteristic *bChar = colorService->createCharacteristic(
        COLOR_CHAR_B_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );
    // bChar->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE);
    bChar->setValue(currentB);
    bChar->setCallbacks(new ColorCallbacks(this, 2));
    // bChar->addDescriptor(createUserDescription("Blue"));

    // --- Яркость ---
    BLECharacteristic *brChar = colorService->createCharacteristic(
        COLOR_CHAR_BRIGHTNESS_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );
    // brChar->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE);
    brChar->setValue(currentBrightness);
    brChar->setCallbacks(new ColorCallbacks(this, 3));
    // brChar->addDescriptor(createUserDescription("Brightness"));

    colorService->start();

    // BLEAdvertising *advertising = BLEDevice::getAdvertising();
    // advertising->addServiceUUID(COLOR_SERVICE_UUID);
}


void LedControl::loadSettings() {
    preferences.begin(LED_NAMESPACE, true);
    currentR = preferences.getUChar("r", RED_DEFAULT);
    currentG = preferences.getUChar("g", GREEN_DEFAULT);
    currentB = preferences.getUChar("b", BLUE_DEFAULT);
    currentBrightness = preferences.getUChar("br", BRIGHT_DEFAULT);
    preferences.end();
}

void LedControl::saveSettings() {
    preferences.begin(LED_NAMESPACE, false);
    preferences.putUChar("r", currentR);
    preferences.putUChar("g", currentG);
    preferences.putUChar("b", currentB);
    preferences.putUChar("br", currentBrightness);
    preferences.end();
}

void LedControl::applyColor() {
    rgbLed.setPixelColor(0, rgbLed.Color(currentR, currentG, currentB));
    rgbLed.setBrightness(correctedBrightness(currentBrightness));
    rgbLed.show();
}

uint8_t LedControl::correctedBrightness(uint8_t percent) {
    if (percent > 100) percent = 100;
    const float gamma = 2.2f;
    return pow(percent / 100.0f, gamma) * 255;
}
void LedControl::forceColor(uint8_t r, uint8_t g, uint8_t b)
{
    rgbLed.setPixelColor(0, rgbLed.Color(r, g, b));
    rgbLed.show();
}
void LedControl::forceBrightness(uint8_t percent)
{
    rgbLed.setBrightness(correctedBrightness(percent));
    rgbLed.show();
}

