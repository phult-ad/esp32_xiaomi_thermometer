# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/mbedtls/port/include $(PROJECT_PATH)/esp-idf/components/mbedtls/mbedtls/include $(PROJECT_PATH)/esp-idf/components/mbedtls/esp_crt_bundle/include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/mbedtls -lmbedtls -Wl,--wrap=mbedtls_mpi_exp_mod
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += $(PROJECT_PATH)/esp-idf/components/mbedtls/mbedtls
COMPONENT_LIBRARIES += mbedtls
COMPONENT_LDFRAGMENTS += 
component-mbedtls-build: 
