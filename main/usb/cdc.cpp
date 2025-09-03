#include "cdc.h"
extern "C" {
#include "class/cdc/cdc_device.h"
}

static cdc_rx_callback_t user_callback = nullptr;

void cdc_send_text(const char* text)
{
    if (tud_cdc_connected()) {
        tud_cdc_write_str(text);
        tud_cdc_write_flush();
    }
}

void cdc_set_rx_callback(cdc_rx_callback_t cb)
{
    user_callback = cb;
}

void cdc_rx_callback()
{
    uint8_t buf[64];
    uint32_t count = tud_cdc_read(buf, sizeof(buf));
    
    if (user_callback) {
        user_callback(buf, count); // Chama o callback do usuário
    }
}