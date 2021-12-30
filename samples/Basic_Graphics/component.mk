COMPONENT_DEPENDS := \
	Graphics \
	RapidXML \
	malloc_count

HWCONFIG := basic-graphics
SPIFF_FILES :=
DISABLE_NETWORK := 1

GLOBAL_CFLAGS += -DENABLE_MALLOC_COUNT=1

RESOURCE_SCRIPT := resource/graphics.rc
