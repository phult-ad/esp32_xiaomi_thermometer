# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/unity/include $(PROJECT_PATH)/esp-idf/components/unity/unity/src
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/unity -lunity
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += $(PROJECT_PATH)/esp-idf/components/unity/unity
COMPONENT_LIBRARIES += unity
COMPONENT_LDFRAGMENTS += 
component-unity-build: 
