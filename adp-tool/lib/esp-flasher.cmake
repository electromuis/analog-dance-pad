set(ESPSERIAL_PATH ${CMAKE_CURRENT_LIST_DIR}/esp-serial-flasher/)

add_library(esp_serial
    ${ESPSERIAL_PATH}port/wjwwood_serial_port.cpp
    ${ESPSERIAL_PATH}src/esp_loader.c
    ${ESPSERIAL_PATH}src/esp_targets.c
    ${ESPSERIAL_PATH}src/esp_stubs.c
    ${ESPSERIAL_PATH}src/md5_hash.c
    ${ESPSERIAL_PATH}src/slip.c
    ${ESPSERIAL_PATH}src/protocol_uart.c
    ${ESPSERIAL_PATH}src/protocol_common.c
)

target_include_directories(esp_serial PUBLIC
    ${ESPSERIAL_PATH}include
    ${ESPSERIAL_PATH}port
)

target_include_directories(esp_serial PRIVATE
    ${ESPSERIAL_PATH}private_include
)

target_compile_definitions(esp_serial PUBLIC
    # -DSERIAL_FLASHER_DEBUG_TRACE
    -DSERIAL_FLASHER_INTERFACE_USB
    -DMD5_ENABLED
    -DSERIAL_FLASHER_WRITE_BLOCK_RETRIES=4
)

target_link_libraries(esp_serial PUBLIC
    serial
)