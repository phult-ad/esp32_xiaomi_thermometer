# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/cbor/port/include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/cbor -lcbor
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += $(PROJECT_PATH)/esp-idf/components/cbor/tinycbor
COMPONENT_LIBRARIES += cbor
COMPONENT_LDFRAGMENTS += 
component-cbor-build: 
