# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(esp32-edge)

# MQTT broker certificate (e.g. https://letsencrypt.org/certs/trustid-x3-root.pem)
target_add_binary_data(esp32-edge.elf "main/mqtt_root.pem" TEXT)
