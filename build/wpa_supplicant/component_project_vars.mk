# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/wpa_supplicant/include $(PROJECT_PATH)/esp-idf/components/wpa_supplicant/port/include $(PROJECT_PATH)/esp-idf/components/wpa_supplicant/esp_supplicant/include $(PROJECT_PATH)/esp-idf/components/wpa_supplicant/src/utils
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/wpa_supplicant -lwpa_supplicant
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += wpa_supplicant
COMPONENT_LDFRAGMENTS += 
component-wpa_supplicant-build: 
