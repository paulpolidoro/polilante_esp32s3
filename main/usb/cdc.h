#pragma once

#include <cstdint>
#include <cstddef>

// Define o tipo do callback
typedef void (*cdc_rx_callback_t)(const uint8_t* data, size_t len);

// Envia texto
void cdc_send_text(const char* text);

// Configura o callback do usuário
void cdc_set_rx_callback(cdc_rx_callback_t cb);


void cdc_rx_callback();