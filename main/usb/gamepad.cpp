#include "gamepad.h"
extern "C" {
#include "class/hid/hid_device.h"
}

void gamepad_send_report(uint16_t buttons, int8_t x, int8_t y, int8_t z)
{
    uint8_t report[5];
    report[0] = buttons & 0xFF;
    report[1] = (buttons >> 8) & 0xFF;
    report[2] = x;
    report[3] = y;
    report[4] = z;
    tud_hid_report(0, report, sizeof(report));
}

// Estado interno
static uint16_t buttons = 0;
static int8_t axis_x = 0, axis_y = 0, axis_z = 0;

void gamepad_init() {
    buttons = 0;
    axis_x = axis_y = axis_z = 0;
    gamepad_send();
}

void gamepad_press(uint8_t button) {
    if (button < 16) {
        buttons |= (1 << button);
        gamepad_send();
    }
}

void gamepad_release(uint8_t button) {
    if (button < 16) {
        buttons &= ~(1 << button);
        gamepad_send();
    }
}

void gamepad_set_x(int8_t x) {
    axis_x = x;
    gamepad_send();
}

void gamepad_set_y(int8_t y) {
    axis_y = y;
    gamepad_send();
}

void gamepad_set_z(int8_t z) {
    axis_z = z;
    gamepad_send();
}

void gamepad_send() {
    uint8_t report[5];
    report[0] = buttons & 0xFF;
    report[1] = (buttons >> 8) & 0xFF;
    report[2] = axis_x;
    report[3] = axis_y;
    report[4] = axis_z;
    tud_hid_report(0, report, sizeof(report));
}