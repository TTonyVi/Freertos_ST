################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/BSP/Src/bsp_button.c \
../App/BSP/Src/bsp_led.c \
../App/BSP/Src/bsp_motor.c \
../App/BSP/Src/bsp_oled.c \
../App/BSP/Src/bsp_sensor.c \
../App/BSP/Src/bsp_valve.c 

OBJS += \
./App/BSP/Src/bsp_button.o \
./App/BSP/Src/bsp_led.o \
./App/BSP/Src/bsp_motor.o \
./App/BSP/Src/bsp_oled.o \
./App/BSP/Src/bsp_sensor.o \
./App/BSP/Src/bsp_valve.o 

C_DEPS += \
./App/BSP/Src/bsp_button.d \
./App/BSP/Src/bsp_led.d \
./App/BSP/Src/bsp_motor.d \
./App/BSP/Src/bsp_oled.d \
./App/BSP/Src/bsp_sensor.d \
./App/BSP/Src/bsp_valve.d 


# Each subdirectory must supply rules for building sources it contributes
App/BSP/Src/%.o App/BSP/Src/%.su App/BSP/Src/%.cyclo: ../App/BSP/Src/%.c App/BSP/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/Drivers/SSD1306" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/OLEDTask/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/SensorTask/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/BSP/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/UITask/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/Common/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/WashingManager/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/SEGGER/Config" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/SEGGER/OS" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/SEGGER/SEGGER" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/FreeRTOS/include" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/FreeRTOS/portable/GCC/ARM_CM4F" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-App-2f-BSP-2f-Src

clean-App-2f-BSP-2f-Src:
	-$(RM) ./App/BSP/Src/bsp_button.cyclo ./App/BSP/Src/bsp_button.d ./App/BSP/Src/bsp_button.o ./App/BSP/Src/bsp_button.su ./App/BSP/Src/bsp_led.cyclo ./App/BSP/Src/bsp_led.d ./App/BSP/Src/bsp_led.o ./App/BSP/Src/bsp_led.su ./App/BSP/Src/bsp_motor.cyclo ./App/BSP/Src/bsp_motor.d ./App/BSP/Src/bsp_motor.o ./App/BSP/Src/bsp_motor.su ./App/BSP/Src/bsp_oled.cyclo ./App/BSP/Src/bsp_oled.d ./App/BSP/Src/bsp_oled.o ./App/BSP/Src/bsp_oled.su ./App/BSP/Src/bsp_sensor.cyclo ./App/BSP/Src/bsp_sensor.d ./App/BSP/Src/bsp_sensor.o ./App/BSP/Src/bsp_sensor.su ./App/BSP/Src/bsp_valve.cyclo ./App/BSP/Src/bsp_valve.d ./App/BSP/Src/bsp_valve.o ./App/BSP/Src/bsp_valve.su

.PHONY: clean-App-2f-BSP-2f-Src

