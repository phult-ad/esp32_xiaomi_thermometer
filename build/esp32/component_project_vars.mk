# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/esp32/include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/esp32 -lesp32 -L $(PROJECT_PATH)/esp-idf/components/esp32/ld -T esp32_out.ld -u ld_include_panic_highint_hdl -T $(BUILD_DIR_BASE)/esp32/esp32.project.ld -T esp32.peripherals.ld 
COMPONENT_LINKER_DEPS += $(PROJECT_PATH)/esp-idf/components/esp32/ld/esp32.peripherals.ld $(BUILD_DIR_BASE)/esp32/esp32.project.ld
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += esp32
COMPONENT_LDFRAGMENTS += $(PROJECT_PATH)/esp-idf/components/esp32/ld/esp32_fragments.lf $(PROJECT_PATH)/esp-idf/components/esp32/linker.lf
component-esp32-build: 
