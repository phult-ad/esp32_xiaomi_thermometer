# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/freertos/include $(PROJECT_PATH)/esp-idf/components/freertos/port/xtensa/include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/freertos -lfreertos -Wl,--undefined=uxTopUsedPriority
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += freertos
COMPONENT_LDFRAGMENTS += $(PROJECT_PATH)/esp-idf/components/freertos/linker.lf
component-freertos-build: 
