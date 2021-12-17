# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/soc/include $(PROJECT_PATH)/esp-idf/components/soc/esp32 $(PROJECT_PATH)/esp-idf/components/soc/esp32/include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/soc -lsoc -T $(PROJECT_PATH)/esp-idf/components/soc/esp32/ld/esp32.peripherals.ld
COMPONENT_LINKER_DEPS += $(PROJECT_PATH)/esp-idf/components/soc $(PROJECT_PATH)/esp-idf/components/soc/esp32/ld/esp32.peripherals.ld
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += soc
COMPONENT_LDFRAGMENTS += $(PROJECT_PATH)/esp-idf/components/soc/linker.lf
component-soc-build: 
