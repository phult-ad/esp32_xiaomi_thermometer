# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/xtensa/include $(PROJECT_PATH)/esp-idf/components/xtensa/esp32/include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/xtensa -lxtensa $(PROJECT_PATH)/esp-idf/components/xtensa/esp32/libxt_hal.a
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += xtensa
COMPONENT_LDFRAGMENTS += $(PROJECT_PATH)/esp-idf/components/xtensa/linker.lf
component-xtensa-build: 
