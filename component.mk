COMPONENT_DEPENDS := \
	HardwareSPI

COMPONENT_SRCDIRS := \
	src \
	src/Drawing \
	src/Display \
	src/Touch \
	src/Arch/$(SMING_ARCH)

COMPONENT_INCDIRS := \
	src/include \
	src/include/Arch/$(SMING_ARCH) \
	resource

COMPONENT_DOXYGEN_INPUT := src/include

#
COMPONENT_RELINK_VARS += ENABLE_GRAPHICS_DEBUG
ENABLE_GRAPHICS_DEBUG ?= 0
ifeq ($(ENABLE_GRAPHICS_DEBUG),1)
COMPONENT_CXXFLAGS += -DENABLE_GRAPHICS_DEBUG=1
endif

#
COMPONENT_VARS += ENABLE_GRAPHICS_RAM_TRACKING
ENABLE_GRAPHICS_RAM_TRACKING ?= 0
ifeq ($(ENABLE_GRAPHICS_RAM_TRACKING),1)
COMPONENT_CXXFLAGS += -DENABLE_GRAPHICS_RAM_TRACKING=1
endif

# Resource compiler
RC_TOOL_CMDLINE := $(PYTHON) -X utf8 $(COMPONENT_PATH)/Tools/rc/rc.py


##@Building

ifdef RESOURCE_SCRIPT
RESDIR := $(PROJECT_DIR)/$(BUILD_BASE)
RESOURCE_FILES := $(RESDIR)/resource.h $(RESDIR)/resource.bin

COMPONENT_PREREQUISITES := $(RESOURCE_FILES)
COMPONENT_INCDIRS += $(RESDIR)

$(RESOURCE_FILES): $(RESOURCE_SCRIPT)
	@echo RC $< $(RESDIR)
	$(Q) $(RC_TOOL_CMDLINE) $(if $(Q),--quiet) $< $(RESDIR)

.PHONY: resource
resource: ##Rebuild resource files
	$(Q) rm -f $(RESOURCE_FILES); $(MAKE) $(RESOURCE_FILES)

endif

#
ifeq ($(SMING_ARCH),Host)

# For application use
CONFIG_VARS += ENABLE_VIRTUAL_SCREEN
CACHE_VARS += VSADDR VSPORT
ENABLE_VIRTUAL_SCREEN ?= 1
VSADDR ?= 192.168.1.105
VSPORT ?= 7780
ifeq ($(ENABLE_VIRTUAL_SCREEN),1)
APP_CFLAGS += -DENABLE_VIRTUAL_SCREEN=1
HOST_PARAMETERS ?= vsaddr=$(VSADDR) vsport=$(VSPORT)
endif

VIRTUAL_SCREEN_PY := $(COMPONENT_PATH)/Tools/vs/screen.py
VIRTUAL_SCREEN_CMDLINE := $(PYTHON) $(VIRTUAL_SCREEN_PY) --localport $(VSPORT)

# When using WSL without an X server available, use native Windows python
ifdef WSL_ROOT
ifndef DISPLAY
VIRTUAL_SCREEN_CMDLINE := powershell.exe -Command "$(VIRTUAL_SCREEN_CMDLINE)"
endif
endif

#
.PHONY: virtual-screen
virtual-screen: ##Start virtual screen server
	$(info Starting virtual screen server)
	$(Q) $(VIRTUAL_SCREEN_CMDLINE) &

endif
