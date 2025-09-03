#include "usb/descriptor.h"
#include "usb/gamepad.h"
#include "usb/cdc.h"
#include "esp_log.h"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "tinyusb.h" 
    #include "ble/ble.h"
}

void my_cdc_rx_handler(const uint8_t* data, size_t len)
{
    // Cria um buffer temporário, garante espaço para o terminador
    char temp[65];
    size_t copy_len = (len < 64) ? len : 64;
    memcpy(temp, data, copy_len);
    temp[copy_len] = '\0'; // Garante string terminada

    cdc_send_text("Recebido: ");
    cdc_send_text(temp);
    cdc_send_text("\r\n");
}


static void steering_cb(const char *data, size_t len) {
    ESP_LOGI("MAIN", "STEERING callback: %.*s", (int)len, data);
}

static void pedals_cb(const char *data, size_t len) {
    ESP_LOGI("MAIN", "PEDALS callback: %.*s", (int)len, data);
}


extern "C" void app_main(void)
{
    usb_init();
    cdc_set_rx_callback(my_cdc_rx_handler);

    ble_init(steering_cb, pedals_cb);


    uint16_t buttons = 0;
    int8_t x = 0, y = 0, z = 0;
    int dir = 0;

    while (1) {
        if (tud_mounted()) {
            buttons = (buttons + 1) & 0xFFFF;
            if (dir == 0) { x = 50; y = 0; z = 0; }
            else if (dir == 1) { x = 0; y = 50; z = 0; }
            else if (dir == 2) { x = 0; y = 0; z = 50; }
            else if (dir == 3) { x = -50; y = 0; z = 0; }
            else if (dir == 4) { x = 0; y = -50; z = 0; }
            else { x = 0; y = 0; z = -50; }
            dir = (dir + 1) % 6;

            gamepad_send_report(buttons, x, y, z);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}