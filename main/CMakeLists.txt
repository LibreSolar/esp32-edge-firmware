
set(app_sources
	"main.c"
	"ts_client.c"
	"ts_serial.c"
	"can.c"
	"emoncms.c"
	"stm32bl.c"
	"wifi.c"
	"web_fs.c"
	"web_server.c"
	"provisioning.c"
)

idf_component_register(SRCS ${app_sources} INCLUDE_DIRS ".")

# Create a SPIFFS image from the contents of the 'webapp' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash'.
spiffs_create_partition_image(website ../webapp/dist FLASH_IN_PROJECT)

# Uncomment to upload STM32 firmware binary saved in ../stm_image/firmware.bin (for testing)
#spiffs_create_partition_image(stm_ota ../stm_image FLASH_IN_PROJECT)
