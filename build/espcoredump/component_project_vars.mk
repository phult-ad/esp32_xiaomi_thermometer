# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/espcoredump/include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/espcoredump -lespcoredump
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += espcoredump
COMPONENT_LDFRAGMENTS += $(PROJECT_PATH)/esp-idf/components/espcoredump/linker.lf
component-espcoredump-build: 
