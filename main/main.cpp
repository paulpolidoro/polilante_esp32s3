#include "usb/descriptor.h"
#include "usb/gamepad.h"
#include "usb/cdc.h"
#include "esp_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "tinyusb.h" 
    #include "ble/ble.h"
}


static const char *TAG = "MAIN";

#define MIN_HALL 1860
#define MAX_HALL 3000


static int8_t map_value(int value) {
    // Garante que o valor esteja dentro do intervalo
    if (value < MIN_HALL) value = MIN_HALL;
    if (value > MAX_HALL) value = MAX_HALL;

    // Regra de 3: (value - MIN) * 254 / (MAX - MIN) - 127
    int mapped = ((value - MIN_HALL) * 254) / (MAX_HALL - MIN_HALL) - 127;
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

    ESP_LOGI("COM", "%s", temp);

    ble_send_pedals(temp,  copy_len);
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
    if (len % 3 != 0) {
        ESP_LOGW(TAG, "Pacote inválido: tamanho não múltiplo de 3");
        return;
    }

    for (size_t i = 0; i < len; i += 3) {
        uint8_t id = data[i];
        uint16_t raw = ((uint8_t)data[i + 1] << 8) | (uint8_t)data[i + 2];
        int8_t mapped = map_value(raw);  // Mapeia para -127 a 127, por exemplo

        switch (id) {
            case 0x01:  // ACC
                gamepad_set_x(mapped);
                //ESP_LOGI(TAG, "ACC: %d", raw);
                break;
            case 0x02:  // BRK
                gamepad_set_y(mapped);
               // ESP_LOGI(TAG, "BRK: %d", raw);
                break;
            case 0x03:  // THT
                gamepad_set_z(mapped);
               // ESP_LOGI(TAG, "THT: %d", raw);
                break;
            default:
                ESP_LOGW(TAG, "ID desconhecido: 0x%02X", id);
                break;
        }
    }
}

// Task dedicada para envio BLE SOMENTE TESTE
void ble_vibration_task(void *pvParameters) {
    srand((unsigned int)time(NULL));

    vTaskDelay(pdMS_TO_TICKS(10000)); 

    while (true) {
        for (uint8_t motor_id = 0x01; motor_id <= 0x03; motor_id++) {
            uint8_t vibration = rand() % 101;
            int result = ble_send_pedal_vibration(motor_id, vibration);

            vTaskDelay(pdMS_TO_TICKS(100));  // Pequeno delay entre motores
        }

        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay entre ciclos
    }
}


extern "C" void app_main(void)
{
    usb_init();
    cdc_set_rx_callback(my_cdc_rx_handler);

    ble_init(steering_cb, pedals_cb);


    xTaskCreate(ble_vibration_task, "BLE_Vibration_Task", 4096, NULL, 5, NULL);


}