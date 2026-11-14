# Add sources to executable/library
target_sources(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/main.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/list.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/queue.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/Scheduler.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/Task.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/portable.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/rtos.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/syscall.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/sysmem.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/startup_stm32l476xx.S"
)

# Copy linker script to the build directory
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/stm32l476xg_flash.ld"
    "${CMAKE_CURRENT_BINARY_DIR}/stm32l476xg_flash.ld"
    COPYONLY
)

# Re-link if the linker script changes
set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/stm32l476xg_flash.ld"
)