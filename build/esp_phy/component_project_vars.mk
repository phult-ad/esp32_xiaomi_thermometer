# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/esp_phy/include $(PROJECT_PATH)/esp-idf/components/esp_phy/esp32/include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/esp_phy -lesp_phy -L$(PROJECT_PATH)/esp-idf/components/esp_phy/lib/esp32 -lphy -lrtc
COMPONENT_LINKER_DEPS += $(PROJECT_PATH)/esp-idf/components/esp_phy/lib/esp32/libphy.a $(PROJECT_PATH)/esp-idf/components/esp_phy/lib/esp32/librtc.a
COMPONENT_SUBMODULES += $(PROJECT_PATH)/esp-idf/components/esp_phy/lib
COMPONENT_LIBRARIES += esp_phy
COMPONENT_LDFRAGMENTS += $(PROJECT_PATH)/esp-idf/components/esp_phy/linker.lf
component-esp_phy-build: 
