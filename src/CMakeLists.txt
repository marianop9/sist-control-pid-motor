add_executable(pwm_test pwm_test.c)

target_link_libraries(pwm_test
    pico_stdlib
    hardware_adc
    hardware_pwm
)

pico_enable_stdio_usb(pwm_test 1)
pico_enable_stdio_uart(pwm_test 0)


add_executable(pid pid_main.c)

target_link_libraries(pid
    pico_stdlib
    hardware_adc
    hardware_pwm
)

pico_enable_stdio_usb(pid 1)
pico_enable_stdio_uart(pid 0)