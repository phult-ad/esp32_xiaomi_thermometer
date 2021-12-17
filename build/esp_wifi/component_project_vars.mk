# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/esp_wifi/include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/esp_wifi -lesp_wifi -L$(PROJECT_PATH)/esp-idf/components/esp_wifi/lib/esp32 -lcore -lnet80211 -lpp -lsmartconfig -lcoexist -lespnow -lmesh
COMPONENT_LINKER_DEPS += $(PROJECT_PATH)/esp-idf/components/esp_wifi/lib/esp32/libcore.a $(PROJECT_PATH)/esp-idf/components/esp_wifi/lib/esp32/libnet80211.a $(PROJECT_PATH)/esp-idf/components/esp_wifi/lib/esp32/libpp.a $(PROJECT_PATH)/esp-idf/components/esp_wifi/lib/esp32/libsmartconfig.a $(PROJECT_PATH)/esp-idf/components/esp_wifi/lib/esp32/libcoexist.a $(PROJECT_PATH)/esp-idf/components/esp_wifi/lib/esp32/libespnow.a $(PROJECT_PATH)/esp-idf/components/esp_wifi/lib/esp32/libmesh.a
COMPONENT_SUBMODULES += $(PROJECT_PATH)/esp-idf/components/esp_wifi/lib
COMPONENT_LIBRARIES += esp_wifi
COMPONENT_LDFRAGMENTS += $(PROJECT_PATH)/esp-idf/components/esp_wifi/linker.lf
component-esp_wifi-build: 
