# Libre Solar Data Manager Firmware

Firmware for CAN and UART to WiFi or Bluetooth gateway based on ESP32-IDF.

**Warning:** This firmware is at a very early stage. Expect bugs and report them in the issues :)

## Supported devices

- Libre Solar [Data Manager](https://github.com/LibreSolar/data-manager)
- Sparkfun ESP32thing
- Many other ESP32-based boards

## Firmware features

- Written in C using ESP-IDF and PlatformIO
- Data input from Libre Solar devices via
    - [LS.bus](https://libre.solar/docs/ls_bus/) (CAN bus)
    - [LS.one](https://libre.solar/docs/ls_one/) (UART serial)
- Forwarding of data via WiFi to
    - Open Energy Monitor [Emoncms](https://emoncms.org/)
    - MQTT sever (ToDo)
- Data logging on SD card (ToDo)

## Usage

This firmware uses PlatformIO for easy bulding and flashing. Before you can compile, you need to
copy the `custom_conf.template.h` to `custom_conf.h` and adjust WiFi and EmonCMS settings
accordingly.
