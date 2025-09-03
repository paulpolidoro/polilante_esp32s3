idf_component_register(
    SRCS "main.cpp"
         "usb/descriptor.cpp"
         "usb/gamepad.cpp"
         "usb/cdc.cpp"
         "ble/ble.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_driver_gpio bt nvs_flash
)