################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/SSD1306/ssd1306.c \
../Drivers/SSD1306/ssd1306_fonts.c 

OBJS += \
./Drivers/SSD1306/ssd1306.o \
./Drivers/SSD1306/ssd1306_fonts.o 

C_DEPS += \
./Drivers/SSD1306/ssd1306.d \
./Drivers/SSD1306/ssd1306_fonts.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/SSD1306/%.o Drivers/SSD1306/%.su Drivers/SSD1306/%.cyclo: ../Drivers/SSD1306/%.c Drivers/SSD1306/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/Drivers/SSD1306" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/BSP/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/Common/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/UITask/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/App/WashingManager/Inc" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/SEGGER/Config" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/SEGGER/OS" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/SEGGER/SEGGER" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/FreeRTOS/include" -I"C:/Users/Dell/STM32CubeIDE/workspace_1.19.0/Freertos/OS/FreeRTOS/portable/GCC/ARM_CM4F" -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-SSD1306

clean-Drivers-2f-SSD1306:
	-$(RM) ./Drivers/SSD1306/ssd1306.cyclo ./Drivers/SSD1306/ssd1306.d ./Drivers/SSD1306/ssd1306.o ./Drivers/SSD1306/ssd1306.su ./Drivers/SSD1306/ssd1306_fonts.cyclo ./Drivers/SSD1306/ssd1306_fonts.d ./Drivers/SSD1306/ssd1306_fonts.o ./Drivers/SSD1306/ssd1306_fonts.su

.PHONY: clean-Drivers-2f-SSD1306

