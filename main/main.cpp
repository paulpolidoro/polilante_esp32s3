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


static const char *TAG = "MAIN";

// Mapeia valor de 0–100 para -127–127
static int8_t map_value(int value) {
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    // regra de 3: (value / 100.0) * 254 - 127
    int mapped = ((value * 254) / 100) - 127;
    return (int8_t)mapped;
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

     ESP_LOGI("COM", "deu boa");

     ble_send_steering(temp,  copy_len);
}


static void steering_cb(const char *data, size_t len) {
    ESP_LOGI(TAG, "STEERING callback: %.*s", (int)len, data);

    char buf[128];
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    memcpy(buf, data, len);
    buf[len] = '\0';

    // Divide pelos ";"
    char *token = strtok(buf, ";");
    while (token != NULL) {
        char key[8];
        int value;

        // Esperado: bXX:0 ou bXX:1
        if (sscanf(token, "%3[^:]:%d", key, &value) == 2) {
            if (key[0] == 'b') {
                int button = atoi(&key[1]);  // pega número após 'b'
                if (button >= 0 && button <= 15) {
                    if (value == 1) {
                        gamepad_press((uint8_t)button);
                        ESP_LOGI(TAG, "Button %d pressed", button);
                    } else {
                        gamepad_release((uint8_t)button);
                        ESP_LOGI(TAG, "Button %d released", button);
                    }
                }
            }
        }

        token = strtok(NULL, ";");
    }
}

static void pedals_cb(const char *data, size_t len) {
     ESP_LOGI(TAG, "PEDALS callback: %.*s", (int)len, data);

    char buf[128];
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    memcpy(buf, data, len);
    buf[len] = '\0';

    // Divide pelos ";"
    char *token = strtok(buf, ";");
    while (token != NULL) {
        char key[8];
        int value;

        if (sscanf(token, "%3[^:]:%d", key, &value) == 2) {
            ESP_LOGI(TAG, "Key=%s Value=%d", key, value);

            if (strcmp(key, "acc") == 0) {
                int8_t mapped = map_value(value);
                gamepad_set_x(mapped);
                ESP_LOGI(TAG, "Mapped acc=%d -> %d", value, mapped);
            } 
            else if (strcmp(key, "brk") == 0) {
                int8_t mapped = map_value(value);
                gamepad_set_y(mapped);
                ESP_LOGI(TAG, "Mapped brk=%d -> %d", value, mapped);
            } 
            else if (strcmp(key, "tht") == 0) {
                int8_t mapped = map_value(value);
                gamepad_set_z(mapped);
                ESP_LOGI(TAG, "Mapped tht=%d -> %d", value, mapped);
            }
        }
        token = strtok(NULL, ";");
    }
}


extern "C" void app_main(void)
{
    usb_init();
    cdc_set_rx_callback(my_cdc_rx_handler);

    ble_init(steering_cb, pedals_cb);


    // uint16_t buttons = 0;
    // int8_t x = 0, y = 0, z = 0;
    // int dir = 0;

    // while (1) {
    //     if (tud_mounted()) {
    //         buttons = (buttons + 1) & 0xFFFF;
    //         if (dir == 0) { x = 50; y = 0; z = 0; }
    //         else if (dir == 1) { x = 0; y = 50; z = 0; }
    //         else if (dir == 2) { x = 0; y = 0; z = 50; }
    //         else if (dir == 3) { x = -50; y = 0; z = 0; }
    //         else if (dir == 4) { x = 0; y = -50; z = 0; }
    //         else { x = 0; y = 0; z = -50; }
    //         dir = (dir + 1) % 6;

    //         gamepad_send_report(buttons, x, y, z);
    //     }

    //     vTaskDelay(pdMS_TO_TICKS(500));
    // }
}