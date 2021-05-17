# SPDX-License-Identifier: Apache-2.0

set(app_sources
	"main.c"
	"ts_client.c"
	"ts_serial.c"
	"ts_mqtt.c"
	"can.c"
	"emoncms.c"
	"stm32bl.c"
	"wifi.c"
	"web_fs.c"
	"web_server.c"
	"provisioning.c"
	"../lib/isotp/isotp.c"
	"isotp_user.c"
	"../lib/thingset/src/thingset.cpp"
	"../lib/thingset/src/thingset_bin.cpp"
	"../lib/thingset/src/thingset_txt.cpp"
	"../lib/thingset/src/cbor.c"
	"../lib/thingset/src/jsmn.c"
	"data_nodes.cpp"
)

idf_component_register(SRCS ${app_sources} INCLUDE_DIRS ".")

# MQTT broker certificate (e.g. https://letsencrypt.org/certs/trustid-x3-root.pem)
target_add_binary_data(${COMPONENT_TARGET} "mqtt_root.pem" TEXT)

# Create a SPIFFS image from the contents of the 'webapp' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash'.
spiffs_create_partition_image(website ../webapp/dist FLASH_IN_PROJECT)

# Uncomment to upload STM32 firmware binary saved in ../stm_image/firmware.bin (for testing)
#spiffs_create_partition_image(stm_ota ../stm_image FLASH_IN_PROJECT)
