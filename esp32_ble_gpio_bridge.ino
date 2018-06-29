#include "esp32-hal-gpio.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

gpio_num_t characteristicToPin(BLECharacteristic* characteristic) {
  return (gpio_num_t)characteristic->getUUID().getNative()->uuid.uuid128[0];
}

boolean isPinValid(int pin) {
  if (pin < 0 || pin >= GPIO_NUM_MAX)
    return false;

  if (pin == 1 || pin == 3) // always high output and won't switch to low; labeled as tx0 and rx0
    return false;

  if (pin >= 6  && pin <= 11) // these seem to disconnect at best and crash device at worst
    return false;

  if (pin == 20 || pin == 24 || pin == 29 || pin == 30 || pin == 31) // returns 0x0 by default and aren't in constants
    return false;

  if (pin >= 34  && pin <= 39) // can't output or pull up
    return false;

  return true;
}

BLEUUID createUUID(BLEUUID baseUUID, uint8_t subID) {
  auto nativeID = *baseUUID.getNative();
  nativeID.uuid.uuid128[0] = subID;
  return BLEUUID(nativeID);
}

class CharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic* characteristic) {
      auto pin = characteristicToPin(characteristic);
      gpio_set_direction(pin, GPIO_MODE_INPUT);
      gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
      delayMicroseconds(100); // required for pin capacitance before read
      auto level = (uint8_t)gpio_get_level(pin);
      characteristic->setValue(&level, 1);
    }

    void onWrite(BLECharacteristic* characteristic) {
      auto pin = characteristicToPin(characteristic);
      auto value = (uint8_t)characteristic->getValue()[0];
      gpio_set_direction(pin, GPIO_MODE_OUTPUT);
      gpio_set_level(pin, value);
    }
};

void setup() {
  BLEDevice::init("ESP32 BLE-GPIO Bridge");

  auto baseUUID = BLEUUID("1117b92a-6922-46d8-8c9e-000000000000");

  auto server = BLEDevice::createServer();

  auto service = server->createService(createUUID(baseUUID, 255), GPIO_NUM_MAX * 3); // number of handles is related to number of characteristics

  auto callbacks = new CharacteristicCallbacks();

  for (auto i = 0; i < GPIO_NUM_MAX; i++) {
    if (isPinValid(i)) {
      gpio_set_direction((gpio_num_t)i, GPIO_MODE_OUTPUT);
      gpio_set_level((gpio_num_t)i, 0);
      auto characteristic = service->createCharacteristic(createUUID(baseUUID, i), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
      characteristic->setCallbacks(callbacks);
    }
  }

  service->start();
  server->getAdvertising()->start();
}

void loop() {
  delay(10000);
}
