# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/esp-idf/components/esp_rom/include $(PROJECT_PATH)/esp-idf/components/esp_rom/esp32 $(PROJECT_PATH)/esp-idf/components/esp_rom/include/esp32
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/esp_rom -lesp_rom -L $(PROJECT_PATH)/esp-idf/components/esp_rom/esp32/ld -T esp32.rom.ld -T esp32.rom.libgcc.ld -T esp32.rom.syscalls.ld -T esp32.rom.newlib-data.ld -T esp32.rom.api.ld -T esp32.rom.newlib-funcs.ld -T esp32.rom.newlib-time.ld -lesp_rom -Wl,--wrap=longjmp 
COMPONENT_LINKER_DEPS += $(PROJECT_PATH)/esp-idf/components/esp_rom/esp32/ld/esp32.rom.ld $(PROJECT_PATH)/esp-idf/components/esp_rom/esp32/ld/esp32.rom.libgcc.ld $(PROJECT_PATH)/esp-idf/components/esp_rom/esp32/ld/esp32.rom.syscalls.ld $(PROJECT_PATH)/esp-idf/components/esp_rom/esp32/ld/esp32.rom.newlib-data.ld $(PROJECT_PATH)/esp-idf/components/esp_rom/esp32/ld/esp32.rom.api.ld $(PROJECT_PATH)/esp-idf/components/esp_rom/esp32/ld/esp32.rom.newlib-funcs.ld $(PROJECT_PATH)/esp-idf/components/esp_rom/esp32/ld/esp32.rom.newlib-time.ld
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += esp_rom
COMPONENT_LDFRAGMENTS += 
component-esp_rom-build: 
