COMPONENT_DEPENDS := \
	Graphics \
	RapidXML \
	malloc_count

HWCONFIG := tft-ili9341
SPIFF_FILES :=

GLOBAL_CFLAGS += -DENABLE_MALLOC_COUNT=1

RESOURCE_SCRIPT := resource/graphics.rc
