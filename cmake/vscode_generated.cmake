# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Compiler options
target_compile_options(${BUILD_UNIT_0_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:C>: ${CUBE_CMAKE_C_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>: ${CUBE_CMAKE_CXX_FLAGS}>
    $<$<COMPILE_LANGUAGE:ASM>: ${CUBE_CMAKE_ASM_FLAGS}>
)

# Linker options
target_link_options(${BUILD_UNIT_0_NAME} PRIVATE ${CUBE_CMAKE_EXE_LINKER_FLAGS})

# Add sources to executable/library
target_sources(${BUILD_UNIT_0_NAME} PRIVATE
    "Core/Src/app_debug.c"
    "Core/Src/app_entry.c"
    "Core/Src/hw_timerserver.c"
    "Core/Src/hw_uart.c"
    "Core/Src/main.c"
    "Core/Src/stm32_lpm_if.c"
    "Core/Src/stm32wbxx_hal_msp.c"
    "Core/Src/stm32wbxx_it.c"
    "Core/Src/syscalls.c"
    "Core/Src/sysmem.c"
    "Core/Src/system_stm32wbxx.c"
    "Core/Startup/startup_stm32wb55vgyx.s"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_cortex.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_dma.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_dma_ex.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_exti.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_flash.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_flash_ex.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_gpio.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_hsem.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_ipcc.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_pwr.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_pwr_ex.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_rcc.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_rcc_ex.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_rtc.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_rtc_ex.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_tim.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_tim_ex.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_uart.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_uart_ex.c"
    "Drivers/STM32WBxx_HAL_Driver/Src/stm32wbxx_ll_rcc.c"
    "Middlewares/ST/STM32_WPAN/ble/core/auto/ble_gap_aci.c"
    "Middlewares/ST/STM32_WPAN/ble/core/auto/ble_gatt_aci.c"
    "Middlewares/ST/STM32_WPAN/ble/core/auto/ble_hal_aci.c"
    "Middlewares/ST/STM32_WPAN/ble/core/auto/ble_hci_le.c"
    "Middlewares/ST/STM32_WPAN/ble/core/auto/ble_l2cap_aci.c"
    "Middlewares/ST/STM32_WPAN/ble/core/template/osal.c"
    "Middlewares/ST/STM32_WPAN/ble/svc/Src/svc_ctl.c"
    "Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/shci/shci.c"
    "Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl/hci_tl.c"
    "Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl/hci_tl_if.c"
    "Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl/shci_tl.c"
    "Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl/shci_tl_if.c"
    "Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl/tl_mbox.c"
    "Middlewares/ST/STM32_WPAN/utilities/dbg_trace.c"
    "Middlewares/ST/STM32_WPAN/utilities/otp.c"
    "Middlewares/ST/STM32_WPAN/utilities/stm_list.c"
    "Middlewares/ST/STM32_WPAN/utilities/stm_queue.c"
    "STM32_WPAN/App/app_ble.c"
    "STM32_WPAN/App/p2p_client_app.c"
    "STM32_WPAN/Target/hw_ipcc.c"
    "Utilities/lpm/tiny_lpm/stm32_lpm.c"
    "Utilities/sequencer/stm32_seq.c"
)

target_include_directories(${BUILD_UNIT_0_NAME} PRIVATE
    "Core/Inc"
    "Drivers/STM32WBxx_HAL_Driver/Inc"
    "Drivers/STM32WBxx_HAL_Driver/Inc/Legacy"
    "Drivers/CMSIS/Device/ST/STM32WBxx/Include"
    "Drivers/CMSIS/Include"
    "STM32_WPAN/App"
    "Utilities/lpm/tiny_lpm"
    "Middlewares/ST/STM32_WPAN"
    "Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread"
    "Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/tl"
    "Middlewares/ST/STM32_WPAN/interface/patterns/ble_thread/shci"
    "Middlewares/ST/STM32_WPAN/utilities"
    "Middlewares/ST/STM32_WPAN/ble/core"
    "Middlewares/ST/STM32_WPAN/ble/core/auto"
    "Middlewares/ST/STM32_WPAN/ble/core/template"
    "Middlewares/ST/STM32_WPAN/ble/svc/Inc"
    "Middlewares/ST/STM32_WPAN/ble/svc/Src"
    "Utilities/sequencer"
    "Middlewares/ST/STM32_WPAN/ble"
)

configure_file("${CMAKE_SOURCE_DIR}/STM32WB55VGYX_FLASH.ld" "${CMAKE_BINARY_DIR}" COPYONLY)

set_target_properties(${BUILD_UNIT_0_NAME} PROPERTIES LINK_DEPENDS "STM32WB55VGYX_FLASH.ld")

