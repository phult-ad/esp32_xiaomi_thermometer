# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/esp_hw_support $(PROJECT_PATH)/esp-idf/components/esp_hw_support/include $(PROJECT_PATH)/esp-idf/components/esp_hw_support/include/soc $(PROJECT_PATH)/esp-idf/components/esp_hw_support/port/esp32 $(PROJECT_PATH)/esp-idf/components/esp_hw_support/port/esp32/private_include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/esp_hw_support -lesp_hw_support
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += esp_hw_support
COMPONENT_LDFRAGMENTS += $(PROJECT_PATH)/esp-idf/components/esp_hw_support/linker.lf
component-esp_hw_support-build: 
