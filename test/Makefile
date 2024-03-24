#
# Build/run Basic_Graphics sample using Host HSPI emulation
#

.NOTPARALLEL:

include $(SMING_HOME)/../Tools/ci/util.mk

MAKE_ARGS := -C ../samples/Basic_Graphics --no-print-directory

execute:
ifeq ($(OS)-$(CI),Windows_NT-true)
	@echo "Skipping test for CI"
else
	$(call TestNotify,Basic_Graphics,start)
	$(Q) $(MAKE) HardwareSPI-clean # May have been build with verbose=3
	$(Q) $(MAKE) $(MAKE_ARGS) \
		DEBUG_VERBOSE_LEVEL=2 \
		ENABLE_VIRTUAL_SCREEN=0 \
		MAX_LOOP_COUNT=1 \
		GUITIMER_INTERVAL=100 \
		SHOWFONT_INTERVAL=100 \
		python-requirements flash run
	$(call TestNotify,Basic_Graphics,success)
endif

%:
	$(Q) $(MAKE) $(MAKE_ARGS) $@
