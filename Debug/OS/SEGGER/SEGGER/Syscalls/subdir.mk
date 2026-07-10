################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../OS/SEGGER/SEGGER/Syscalls/SEGGER_RTT_Syscalls_GCC.c 

OBJS += \
./OS/SEGGER/SEGGER/Syscalls/SEGGER_RTT_Syscalls_GCC.o 

C_DEPS += \
./OS/SEGGER/SEGGER/Syscalls/SEGGER_RTT_Syscalls_GCC.d 


# Each subdirectory must supply rules for building sources it contributes
OS/SEGGER/SEGGER/Syscalls/%.o OS/SEGGER/SEGGER/Syscalls/%.su OS/SEGGER/SEGGER/Syscalls/%.cyclo: ../OS/SEGGER/SEGGER/Syscalls/%.c OS/SEGGER/SEGGER/Syscalls/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/BSP/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/Common/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/OLEDTask/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/SensorTask/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/UITask/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/WashingManager/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/Drivers/SSD1306" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/BSP/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/Common/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/UITask/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/WashingManager/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/SEGGER/Config" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/SEGGER/OS" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/SEGGER/SEGGER" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/FreeRTOS/include" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/FreeRTOS/portable/GCC/ARM_CM4F" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-OS-2f-SEGGER-2f-SEGGER-2f-Syscalls

clean-OS-2f-SEGGER-2f-SEGGER-2f-Syscalls:
	-$(RM) ./OS/SEGGER/SEGGER/Syscalls/SEGGER_RTT_Syscalls_GCC.cyclo ./OS/SEGGER/SEGGER/Syscalls/SEGGER_RTT_Syscalls_GCC.d ./OS/SEGGER/SEGGER/Syscalls/SEGGER_RTT_Syscalls_GCC.o ./OS/SEGGER/SEGGER/Syscalls/SEGGER_RTT_Syscalls_GCC.su

.PHONY: clean-OS-2f-SEGGER-2f-SEGGER-2f-Syscalls

