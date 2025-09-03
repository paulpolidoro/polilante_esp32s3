#pragma once
#include <cstdint>

void gamepad_send_report(uint16_t buttons, int8_t x, int8_t y, int8_t z);

void gamepad_init();
void gamepad_press(uint8_t button);
void gamepad_release(uint8_t button);
void gamepad_set_x(int8_t x); //-127 até 127
void gamepad_set_y(int8_t y); //-127 até 127
void gamepad_set_z(int8_t z); //-127 até 127
void gamepad_send();