# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/lwip/include/apps $(PROJECT_PATH)/esp-idf/components/lwip/include/apps/sntp $(PROJECT_PATH)/esp-idf/components/lwip/lwip/src/include $(PROJECT_PATH)/esp-idf/components/lwip/port/esp32/include $(PROJECT_PATH)/esp-idf/components/lwip/port/esp32/include/arch
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/lwip -llwip
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += $(PROJECT_PATH)/esp-idf/components/lwip/lwip
COMPONENT_LIBRARIES += lwip
COMPONENT_LDFRAGMENTS += $(PROJECT_PATH)/esp-idf/components/lwip/linker.lf
component-lwip-build: 
