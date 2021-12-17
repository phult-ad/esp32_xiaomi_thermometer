# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/efuse/include $(PROJECT_PATH)/esp-idf/components/efuse/esp32/include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/efuse -lefuse
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += efuse
COMPONENT_LDFRAGMENTS += 
component-efuse-build: 
