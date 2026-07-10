################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/Common/Src/freertos_objects.c 

OBJS += \
./App/Common/Src/freertos_objects.o 

C_DEPS += \
./App/Common/Src/freertos_objects.d 


# Each subdirectory must supply rules for building sources it contributes
App/Common/Src/%.o App/Common/Src/%.su App/Common/Src/%.cyclo: ../App/Common/Src/%.c App/Common/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/Drivers/SSD1306" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/OLEDTask/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/SensorTask/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/BSP/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/UITask/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/Common/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/WashingManager/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/SEGGER/Config" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/SEGGER/OS" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/SEGGER/SEGGER" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/FreeRTOS/include" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/FreeRTOS/portable/GCC/ARM_CM4F" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App-2f-Common-2f-Src

clean-App-2f-Common-2f-Src:
	-$(RM) ./App/Common/Src/freertos_objects.cyclo ./App/Common/Src/freertos_objects.d ./App/Common/Src/freertos_objects.o ./App/Common/Src/freertos_objects.su

.PHONY: clean-App-2f-Common-2f-Src

