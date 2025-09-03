#include "descriptor.h"
#include "gamepad.h"
#include "cdc.h"
extern "C" {
#include "tinyusb.h"
#include "esp_log.h"
#include "class/hid/hid_device.h"
#include "class/cdc/cdc_device.h"
}

#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN)

const uint8_t hid_report_descriptor[] = {
    0x05, 0x01, 0x09, 0x05, 0xA1, 0x01, 0xA1, 0x00,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x10, 0x15, 0x00,
    0x25, 0x01, 0x95, 0x10, 0x75, 0x01, 0x81, 0x02,
    0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x32,
    0x15, 0x81, 0x25, 0x7F, 0x75, 0x08, 0x95, 0x03,
    0x81, 0x02, 0xC0, 0xC0
};

const uint16_t lang_id[] = {0x0409};
const char* hid_string_descriptor[5] = {
    reinterpret_cast<const char*>(lang_id),
    "Polilantes",
    "Hub Polilante",
    "123456",
    "Gamepad HID"
};

const uint8_t hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 3, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_CDC_DESCRIPTOR(0, 0, 0x82, 8, 0x01, 0x83, 64),
    TUD_HID_DESCRIPTOR(2, 0, false, sizeof(hid_report_descriptor), 0x81, 16, 10)
};

// HID callbacks
extern "C" uint8_t const *tud_hid_descriptor_report_cb(uint8_t) {
    return hid_report_descriptor;
}
extern "C" uint16_t tud_hid_get_report_cb(uint8_t, uint8_t, hid_report_type_t, uint8_t*, uint16_t) {
    return 0;
}
extern "C" void tud_hid_set_report_cb(uint8_t, uint8_t, hid_report_type_t, uint8_t const*, uint16_t) {}

// CDC callback
extern "C" void tud_cdc_rx_cb(uint8_t) {
    cdc_rx_callback();
}

void usb_init()
{
    static const char *TAG = "usb_config";
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,
        .string_descriptor = hid_string_descriptor,
        .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
        .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
        .fs_configuration_descriptor = hid_configuration_descriptor,
        .hs_configuration_descriptor = hid_configuration_descriptor,
        .qualifier_descriptor = NULL,
#else
        .configuration_descriptor = hid_configuration_descriptor,
#endif
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB initialization DONE");
}