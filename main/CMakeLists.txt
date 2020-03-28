
set(app_sources
	"can.c"
	"main.c"
	"serial.c"
	"emoncms.c"
	"wifi.c"
	"web_fs.c"
)

idf_component_register(SRCS ${app_sources} INCLUDE_DIRS ".")

# Create a SPIFFS image from the contents of the 'webapp' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash'.
spiffs_create_partition_image(storage ../webapp FLASH_IN_PROJECT)
