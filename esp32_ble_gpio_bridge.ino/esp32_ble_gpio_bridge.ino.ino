#include "esp32-hal-gpio.h"

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

class CharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic* characteristic) {
      auto pin = (gpio_num_t)characteristic->getUUID().getNative()->uuid.uuid128[0];
      gpio_set_direction(pin, GPIO_MODE_INPUT);
      gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
      auto level = (uint8_t)gpio_get_level(pin);
      characteristic->setValue(&level, 1);
    }

    void onWrite(BLECharacteristic *characteristic) {
      auto pin = (gpio_num_t)characteristic->getUUID().getNative()->uuid.uuid128[0];
      auto value = (uint8_t)characteristic->getValue()[0];
      gpio_set_direction(pin, GPIO_MODE_OUTPUT);
      gpio_set_level(pin, value);
    }
};

void setup() {
  BLEDevice::init("BLE-GPIO Bridge");

  auto server = BLEDevice::createServer();

  auto service = server->createService(BLEUUID("b350d77d-68bf-49f4-b58d-4f3ea9af0588"), GPIO_NUM_MAX * 3); // number of handles is related to number of characteristics

  auto baseCharacteristicID = BLEUUID("1117b92a-6922-46d8-8c9e-000000000000");

  auto callbacks = new CharacteristicCallbacks();

  for (auto i = 0; i < GPIO_NUM_MAX; i++) {
    auto nativeCharacteristicID = *baseCharacteristicID.getNative();
    nativeCharacteristicID.uuid.uuid128[0] = i;

    auto characteristic = service->createCharacteristic( BLEUUID(nativeCharacteristicID), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
    characteristic->setCallbacks(callbacks); ESP_GATT_UUID_HID_PROTO_MODE;
  }

  service->start();
  server->getAdvertising()->start();
}

void loop() {
  delay(10000);
}
