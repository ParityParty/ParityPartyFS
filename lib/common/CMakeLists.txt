set(NAME common)

add_library(${NAME} INTERFACE)

target_include_directories(${NAME} INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(ENABLE_FREERTOS)
    target_compile_definitions(${NAME} INTERFACE PPFS_USE_FREERTOS)
    target_link_libraries(${NAME} INTERFACE freertos_config FreeRTOS-Kernel-Heap4)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(${NAME} INTERFACE Threads::Threads)
endif()