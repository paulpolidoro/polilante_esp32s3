/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <cstdlib>
extern "C" {
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "class/hid/hid_device.h"
#include "class/cdc/cdc_device.h"
}

static const char *TAG = "example";

/************* TinyUSB descriptors ****************/

#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN)

/**
 * @brief HID report descriptor
 *
 * Gamepad HID com 16 botões e 3 eixos (X/Y/Z)
 */
const uint8_t hid_report_descriptor[] = {
    0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,       // USAGE (Game Pad)
    0xA1, 0x01,       // COLLECTION (Application)
    0xA1, 0x00,       //   COLLECTION (Physical)
    0x05, 0x09,       //     USAGE_PAGE (Button)
    0x19, 0x01,       //     USAGE_MINIMUM (Button 1)
    0x29, 0x10,       //     USAGE_MAXIMUM (Button 16)
    0x15, 0x00,       //     LOGICAL_MINIMUM (0)
    0x25, 0x01,       //     LOGICAL_MAXIMUM (1)
    0x95, 0x10,       //     REPORT_COUNT (16)
    0x75, 0x01,       //     REPORT_SIZE (1)
    0x81, 0x02,       //     INPUT (Data,Var,Abs)
    0x05, 0x01,       //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,       //     USAGE (X)
    0x09, 0x31,       //     USAGE (Y)
    0x09, 0x32,       //     USAGE (Z)
    0x15, 0x81,       //     LOGICAL_MINIMUM (-127)
    0x25, 0x7F,       //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,       //     REPORT_SIZE (8)
    0x95, 0x03,       //     REPORT_COUNT (3)
    0x81, 0x02,       //     INPUT (Data,Var,Abs)
    0xC0,             //   END_COLLECTION
    0xC0              // END_COLLECTION
};

/**
 * @brief String descriptor
 */
const uint16_t lang_id[] = {0x0409}; // English
const char* hid_string_descriptor[5] = {
    reinterpret_cast<const char*>(lang_id), // 0: Idioma
    "TinyUSB",             // 1: Manufacturer
    "ESP32-S3 Gamepad",    // 2: Product
    "123456",              // 3: Serial
    "Gamepad HID",         // 4: HID
};

/**
 * @brief Configuration descriptor (CDC + HID)
 */
const uint8_t hid_configuration_descriptor[] = {
    // CDC ocupa 2 interfaces (0 e 1), HID é a interface 2
    TUD_CONFIG_DESCRIPTOR(1, 3, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_CDC_DESCRIPTOR(0, 0, 0x82, 8, 0x01, 0x83, 64), // CDC nas interfaces 0 e 1
    TUD_HID_DESCRIPTOR(2, 0, false, sizeof(hid_report_descriptor), 0x81, 16, 10) // HID na interface 2
};

/********* TinyUSB HID callbacks ***************/

// Invoked when received GET HID REPORT DESCRIPTOR request
extern "C" uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    return hid_report_descriptor;
}

// Invoked when received GET_REPORT control request
extern "C" uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;
    return 0;
}

// Invoked when received SET_REPORT control request or OUT endpoint
extern "C" void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    // Não utilizado
}

/********* CDC callbacks ***************/

// Callback chamado quando recebe dados pela porta serial USB
extern "C" void tud_cdc_rx_cb(uint8_t itf)
{
    uint8_t buf[64];
    uint32_t count = tud_cdc_read(buf, sizeof(buf));
    // Apenas ecoa o que recebeu
    tud_cdc_write(buf, count);
    tud_cdc_write_flush();
}

/********* Application ***************/

// Função para enviar relatório de gamepad
static void app_send_gamepad_report(uint16_t buttons, int8_t x, int8_t y, int8_t z)
{
    // Relatório: [botões (16 bits), x, y, z]
    uint8_t report[5];
    report[0] = buttons & 0xFF;        // Botões 0-7
    report[1] = (buttons >> 8) & 0xFF; // Botões 8-15
    report[2] = x;                     // eixo X
    report[3] = y;                     // eixo Y
    report[4] = z;                     // eixo Z

    tud_hid_report(0, report, sizeof(report));
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "USB initialization");
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
#endif // TUD_OPT_HIGH_SPEED
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB initialization DONE");

    uint16_t buttons = 0;
    int8_t x = 0;
    int8_t y = 0;
    int8_t z = 0;
    int dir = 0;

    while (1) {
        if (tud_mounted()) {
            // Simula botões mudando a cada ciclo
            buttons = (buttons + 1) & 0xFFFF; // alterna botões 0-15
            // Simula movimento em X/Y/Z
            if (dir == 0) { x = 50; y = 0; z = 0; }
            else if (dir == 1) { x = 0; y = 50; z = 0; }
            else if (dir == 2) { x = 0; y = 0; z = 50; }
            else if (dir == 3) { x = -50; y = 0; z = 0; }
            else if (dir == 4) { x = 0; y = -50; z = 0; }
            else { x = 0; y = 0; z = -50; }
            dir = (dir + 1) % 6;

            app_send_gamepad_report(buttons, x, y, z);

            // CDC Serial: envia texto a cada ciclo
            if (tud_cdc_connected()) {
                tud_cdc_write_str("Hello from ESP32-S3 CDC!\r\n");
                tud_cdc_write_flush();
            }

            ESP_LOGI(TAG, "Gamepad report: buttons=%04X, x=%d, y=%d, z=%d", buttons, x, y, z);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}