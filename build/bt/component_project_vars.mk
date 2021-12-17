# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/bt/include $(PROJECT_PATH)/esp-idf/components/bt/include/esp32/include $(PROJECT_PATH)/esp-idf/components/bt/common/api/include/api $(PROJECT_PATH)/esp-idf/components/bt/common/btc/profile/esp/blufi/include $(PROJECT_PATH)/esp-idf/components/bt/common/btc/profile/esp/include $(PROJECT_PATH)/esp-idf/components/bt/common/osi/include $(PROJECT_PATH)/esp-idf/components/bt/host/bluedroid/api/include/api
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/bt -lbt -L $(PROJECT_PATH)/esp-idf/components/bt/controller/lib_esp32/esp32 -u ld_include_hli_vectors_bt -lbtdm_app
COMPONENT_LINKER_DEPS += $(PROJECT_PATH)/esp-idf/components/bt/controller/lib_esp32/esp32/libbtdm_app.a
COMPONENT_SUBMODULES += $(PROJECT_PATH)/esp-idf/components/bt/controller/lib_esp32
COMPONENT_LIBRARIES += bt
COMPONENT_LDFRAGMENTS += $(PROJECT_PATH)/esp-idf/components/bt/linker.lf
component-bt-build: 
