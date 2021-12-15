# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/esp_gdbstub/include $(PROJECT_PATH)/esp-idf/components/esp_gdbstub/xtensa $(PROJECT_PATH)/esp-idf/components/esp_gdbstub/esp32
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/esp_gdbstub -lesp_gdbstub
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += esp_gdbstub
COMPONENT_LDFRAGMENTS += $(PROJECT_PATH)/esp-idf/components/esp_gdbstub/linker.lf
component-esp_gdbstub-build: 
