#include "esp32-hal-gpio.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

gpio_num_t characteristicToPin(BLECharacteristic* characteristic) {
  return (gpio_num_t)characteristic->getUUID().getNative()->uuid.uuid128[0];
}

boolean isPinValid(gpio_num_t pin) {
  if (pin > 1 && pin < 20)
    return true;
}

void setup() {
  BLEDevice::init("BLE-GPIO Bridge");

  auto server = BLEDevice::createServer();

  auto service = server->createService(BLEUUID("b350d77d-68bf-49f4-b58d-4f3ea9af0588"), GPIO_NUM_MAX * 3); // number of handles is related to number of characteristics

  auto baseCharacteristicID = BLEUUID("1117b92a-6922-46d8-8c9e-000000000000");

class: public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic* characteristic) {
      auto pin = characteristicToPin(characteristic);
      gpio_set_direction(pin, GPIO_MODE_INPUT);
      gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
      auto level = (uint8_t)gpio_get_level(pin);
      characteristic->setValue(&level, 1);
    }

    void onWrite(BLECharacteristic* characteristic) {
      auto pin = characteristicToPin(characteristic);
      auto value = (uint8_t)characteristic->getValue()[0];
      gpio_set_direction(pin, GPIO_MODE_OUTPUT);
      gpio_set_level(pin, value);
    }
} callbacks;

  for (auto i = 0; i < GPIO_NUM_MAX; i++) {
    if (isPinValid(i)) {
      auto nativeCharacteristicID = *baseCharacteristicID.getNative();
      nativeCharacteristicID.uuid.uuid128[0] = i;

      auto characteristic = service->createCharacteristic( BLEUUID(nativeCharacteristicID), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
      characteristic->setCallbacks(callbacks);
    }
  }

  service->start();
  server->getAdvertising()->start();
}

void loop() {
  delay(10000);
}
