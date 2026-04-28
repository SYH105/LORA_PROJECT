################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FLASH/FLASH.c 

OBJS += \
./FLASH/FLASH.o 

C_DEPS += \
./FLASH/FLASH.d 


# Each subdirectory must supply rules for building sources it contributes
FLASH/%.o FLASH/%.su FLASH/%.cyclo: ../FLASH/%.c FLASH/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I"C:/Users/YONG/STM32CubeIDE/workspace_1.18.0/LORA_PROJECT/FLASH" -I"C:/Users/YONG/STM32CubeIDE/workspace_1.18.0/LORA_PROJECT/APP" -I"C:/Users/YONG/STM32CubeIDE/workspace_1.18.0/LORA_PROJECT/LORA" -I"C:/Users/YONG/STM32CubeIDE/workspace_1.18.0/LORA_PROJECT/BUTTON" -I"C:/Users/YONG/STM32CubeIDE/workspace_1.18.0/LORA_PROJECT/UI" -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-FLASH

clean-FLASH:
	-$(RM) ./FLASH/FLASH.cyclo ./FLASH/FLASH.d ./FLASH/FLASH.o ./FLASH/FLASH.su

.PHONY: clean-FLASH

