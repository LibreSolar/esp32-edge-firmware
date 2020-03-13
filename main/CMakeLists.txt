
set(app_sources
	"can.c"
	"main.c"
	"serial.c"
)

idf_component_register(SRCS ${app_sources} INCLUDE_DIRS ".")
