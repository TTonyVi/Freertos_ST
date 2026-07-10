################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../OS/SEGGER/SEGGER/SEGGER_RTT.c \
../OS/SEGGER/SEGGER/SEGGER_RTT_printf.c \
../OS/SEGGER/SEGGER/SEGGER_SYSVIEW.c 

S_UPPER_SRCS += \
../OS/SEGGER/SEGGER/SEGGER_RTT_ASM_ARMv7M.S 

OBJS += \
./OS/SEGGER/SEGGER/SEGGER_RTT.o \
./OS/SEGGER/SEGGER/SEGGER_RTT_ASM_ARMv7M.o \
./OS/SEGGER/SEGGER/SEGGER_RTT_printf.o \
./OS/SEGGER/SEGGER/SEGGER_SYSVIEW.o 

S_UPPER_DEPS += \
./OS/SEGGER/SEGGER/SEGGER_RTT_ASM_ARMv7M.d 

C_DEPS += \
./OS/SEGGER/SEGGER/SEGGER_RTT.d \
./OS/SEGGER/SEGGER/SEGGER_RTT_printf.d \
./OS/SEGGER/SEGGER/SEGGER_SYSVIEW.d 


# Each subdirectory must supply rules for building sources it contributes
OS/SEGGER/SEGGER/%.o OS/SEGGER/SEGGER/%.su OS/SEGGER/SEGGER/%.cyclo: ../OS/SEGGER/SEGGER/%.c OS/SEGGER/SEGGER/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/BSP/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/Common/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/OLEDTask/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/SensorTask/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/UITask/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/WashingManager/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/Drivers/SSD1306" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/BSP/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/Common/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/UITask/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/WashingManager/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/SEGGER/Config" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/SEGGER/OS" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/SEGGER/SEGGER" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/FreeRTOS/include" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/FreeRTOS/portable/GCC/ARM_CM4F" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
OS/SEGGER/SEGGER/%.o: ../OS/SEGGER/SEGGER/%.S OS/SEGGER/SEGGER/subdir.mk
	arm-none-eabi-gcc -mcpu=cortex-m4 -g3 -DDEBUG -c -I../Core/Inc -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/SEGGER/Config" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@" "$<"

clean: clean-OS-2f-SEGGER-2f-SEGGER

clean-OS-2f-SEGGER-2f-SEGGER:
	-$(RM) ./OS/SEGGER/SEGGER/SEGGER_RTT.cyclo ./OS/SEGGER/SEGGER/SEGGER_RTT.d ./OS/SEGGER/SEGGER/SEGGER_RTT.o ./OS/SEGGER/SEGGER/SEGGER_RTT.su ./OS/SEGGER/SEGGER/SEGGER_RTT_ASM_ARMv7M.d ./OS/SEGGER/SEGGER/SEGGER_RTT_ASM_ARMv7M.o ./OS/SEGGER/SEGGER/SEGGER_RTT_printf.cyclo ./OS/SEGGER/SEGGER/SEGGER_RTT_printf.d ./OS/SEGGER/SEGGER/SEGGER_RTT_printf.o ./OS/SEGGER/SEGGER/SEGGER_RTT_printf.su ./OS/SEGGER/SEGGER/SEGGER_SYSVIEW.cyclo ./OS/SEGGER/SEGGER/SEGGER_SYSVIEW.d ./OS/SEGGER/SEGGER/SEGGER_SYSVIEW.o ./OS/SEGGER/SEGGER/SEGGER_SYSVIEW.su

.PHONY: clean-OS-2f-SEGGER-2f-SEGGER

