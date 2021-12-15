# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/hal/esp32/include $(PROJECT_PATH)/esp-idf/components/hal/include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/hal -lhal
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += hal
COMPONENT_LDFRAGMENTS += $(PROJECT_PATH)/esp-idf/components/hal/linker.lf
component-hal-build: 
