# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/esp_system/include $(PROJECT_PATH)/esp-idf/components/esp_system/port/public_compat
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/esp_system -lesp_system -u __ubsan_include -u ld_include_highint_hdl -T $(BUILD_DIR_BASE)/esp_system/ld/memory.ld -T $(BUILD_DIR_BASE)/esp_system/ld/sections.ld 
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += esp_system
COMPONENT_LDFRAGMENTS += $(PROJECT_PATH)/esp-idf/components/esp_system/linker.lf $(PROJECT_PATH)/esp-idf/components/esp_system/app.lf
component-esp_system-build: component-esp_phy-build
