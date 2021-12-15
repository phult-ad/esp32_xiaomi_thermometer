# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/esp-tls $(PROJECT_PATH)/esp-idf/components/esp-tls/esp-tls-crypto $(PROJECT_PATH)/esp-idf/components/esp-tls/private_include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/esp-tls -lesp-tls
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += esp-tls
COMPONENT_LDFRAGMENTS += 
component-esp-tls-build: 
