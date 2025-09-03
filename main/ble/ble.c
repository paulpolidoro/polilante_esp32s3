#include "ble.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "BLE";

#define MAX_CONN 2
static uint16_t conn_handles[MAX_CONN] = {0};

static ble_rx_callback_t steering_rx_cb = NULL;
static ble_rx_callback_t pedals_rx_cb   = NULL;

static uint16_t steering_handle;
static uint16_t pedals_handle;

// UUIDs

// SERVICE
static const ble_uuid16_t service_uuid = BLE_UUID16_INIT(0xAB10);

// STEERING 128-bit (substitua se quiser usar 16-bit)
static const ble_uuid16_t steering_uuid = BLE_UUID16_INIT(0xAB11);

// PEDALS 16-bit
static const ble_uuid16_t pedals_uuid = BLE_UUID16_INIT(0xAB12);

// Buffers
static char steering_buf[50] = {0};
static char pedals_buf[50]   = {0};

// Callback escrita STEERING
static int char_steering_cb(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    size_t len = OS_MBUF_PKTLEN(ctxt->om);
    if (len < sizeof(steering_buf)) {
        ble_hs_mbuf_to_flat(ctxt->om, steering_buf, len, NULL);
        steering_buf[len] = '\0';
        ESP_LOGI(TAG, "STEERING received: %s", steering_buf);
        if (steering_rx_cb) steering_rx_cb(steering_buf, len);
    }
    return 0;
}

// Callback escrita PEDALS
static int char_pedals_cb(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    size_t len = OS_MBUF_PKTLEN(ctxt->om);
    if (len < sizeof(pedals_buf)) {
        ble_hs_mbuf_to_flat(ctxt->om, pedals_buf, len, NULL);
        pedals_buf[len] = '\0';
        ESP_LOGI(TAG, "PEDALS received: %s", pedals_buf);
        if (pedals_rx_cb) pedals_rx_cb(pedals_buf, len);
    }
    return 0;
}

// GATT
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &service_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &steering_uuid.u,
                .access_cb = char_steering_cb,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &steering_handle,
            },
            {
                .uuid = &pedals_uuid.u,
                .access_cb = char_pedals_cb,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &pedals_handle,
            },
            {0}
        }
    },
    {0}
};

// GAP
static int gap_event_handler(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "Dispositivo conectado");

                // Armazena handle na primeira posição livre
                for (int i = 0; i < MAX_CONN; i++) {
                    if (conn_handles[i] == 0) {
                        conn_handles[i] = event->connect.conn_handle;
                        break;
                    }
                }

                        // Verifica se ainda há slots livres e reinicia advertising
                bool has_free_slot = false;
                for (int i = 0; i < MAX_CONN; i++) {
                    if (conn_handles[i] == 0) {
                        has_free_slot = true;
                        break;
                    }
                }
                if (has_free_slot) {
                    struct ble_gap_adv_params adv_params = {
                        .conn_mode = BLE_GAP_CONN_MODE_UND,
                        .disc_mode = BLE_GAP_DISC_MODE_GEN,
                    };
                    struct ble_hs_adv_fields fields = {
                        .flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP,
                        .name = (uint8_t *)"ESP32-S3-NimBLE",
                        .name_len = strlen("ESP32-S3-NimBLE"),
                        .name_is_complete = 1,
                    };
                    ble_gap_adv_set_fields(&fields);
                    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                                    &adv_params, gap_event_handler, NULL);
                }

            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Dispositivo desconectado");

            // Limpa handle correspondente
            for (int i = 0; i < MAX_CONN; i++) {
                if (conn_handles[i] != event->connect.conn_handle) {
                    conn_handles[i] = 0;
                    break;
                }
            }

            // Reinicia advertising
            {
                struct ble_gap_adv_params adv_params = {
                    .conn_mode = BLE_GAP_CONN_MODE_UND,
                    .disc_mode = BLE_GAP_DISC_MODE_GEN,
                };
                struct ble_hs_adv_fields fields = {
                    .flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP,
                    .name = (uint8_t *)"ESP32-S3-NimBLE",
                    .name_len = strlen("ESP32-S3-NimBLE"),
                    .name_is_complete = 1,
                };
                ble_gap_adv_set_fields(&fields);
                ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                                  &adv_params, gap_event_handler, NULL);
            }
            break;

        default:
            break;
    }
    return 0;
}

// Sync
static void ble_app_on_sync(void)
{
    struct ble_hs_adv_fields fields = {
        .flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP,
        .name = (uint8_t *)"ESP32-S3-NimBLE",
        .name_len = strlen("ESP32-S3-NimBLE"),
        .name_is_complete = 1,
    };
    ble_gap_adv_set_fields(&fields);

    struct ble_gap_adv_params adv_params = {
        .conn_mode = BLE_GAP_CONN_MODE_UND,
        .disc_mode = BLE_GAP_DISC_MODE_GEN,
    };
    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                      &adv_params, gap_event_handler, NULL);

    ESP_LOGI(TAG, "Advertising iniciado");
}

// Host task
static void host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

// Inicialização
void ble_init(ble_rx_callback_t steering_cb_in, ble_rx_callback_t pedals_cb_in)
{
    steering_rx_cb = steering_cb_in;
    pedals_rx_cb   = pedals_cb_in;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    ESP_LOGI(TAG, "Inicializando NimBLE...");

    nimble_port_init();
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);

    ble_hs_cfg.sync_cb = ble_app_on_sync;
    nimble_port_freertos_init(host_task);

    ESP_LOGI(TAG, "NimBLE inicializado com sucesso");

    ESP_LOGI(TAG, "Handle da característica steering: %d", steering_handle);
    ESP_LOGI(TAG, "Handle da característica pedals: %d", pedals_handle);
}

// Envio STEERING
int ble_send_steering(const char *data, size_t len)
{
    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (!om) return -1;

    int rc = 0;
    for (int i = 0; i < MAX_CONN; i++) {
        if (conn_handles[i]) {
            rc = ble_gattc_notify_custom(conn_handles[i], steering_handle, om);
        }
    }
    return rc;
}

// Envio PEDALS
int ble_send_pedals(const char *data, size_t len)
{
    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (!om) return -1;

    int rc = 0;
    for (int i = 0; i < MAX_CONN; i++) {
        if (conn_handles[i]) {
            rc = ble_gattc_notify_custom(conn_handles[i], pedals_handle, om);
        }
    }
    return rc;
}
