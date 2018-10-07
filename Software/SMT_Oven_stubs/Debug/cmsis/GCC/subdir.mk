################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
../cmsis/GCC/HAL_CM4.S \
../cmsis/GCC/SVC_Table.S 

OBJS += \
./cmsis/GCC/HAL_CM4.o \
./cmsis/GCC/SVC_Table.o 


# Each subdirectory must supply rules for building sources it contributes
cmsis/GCC/%.o: ../cmsis/GCC/%.S
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Assembler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -g3 -O0 -ffunction-sections -fdata-sections -finline-limit=100 -Wall -Wextra -x assembler-with-cpp -DDEBUG_BUILD -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/Project_Headers" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/cmsis" -Wall -Wextra -c -fmessage-length=0  -o "$@" $<
	@echo 'Finished building: $<'
	@echo ' '


