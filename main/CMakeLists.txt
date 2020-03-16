
set(app_sources
	"can.c"
	"main.c"
	"serial.c"
	"emoncms.c"
	"wifi.c"
)

idf_component_register(SRCS ${app_sources} INCLUDE_DIRS ".")
