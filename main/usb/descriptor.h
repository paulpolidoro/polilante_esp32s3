#pragma once
#include <cstdint>

extern const uint8_t hid_report_descriptor[];
extern const char* hid_string_descriptor[5];
extern const uint8_t hid_configuration_descriptor[];

void usb_init();