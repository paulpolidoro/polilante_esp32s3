#ifndef BLE_H
#define BLE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ble_rx_callback_t)(const char *data, size_t len);

// Inicializa BLE e registra callbacks para cada característica
void ble_init(ble_rx_callback_t steering_cb, ble_rx_callback_t pedals_cb);

// Envia dados via notify para STEERING
int ble_send_steering(const char *data, size_t len);

// Envia dados via notify para PEDALS
int ble_send_pedals(const char *data, size_t len);

int ble_send_pedal_vibration(uint8_t id, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif // BLE_H
