################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Startup_Code/console.cpp \
../Startup_Code/mcg.cpp \
../Startup_Code/rtc.cpp \
../Startup_Code/system-gcc.cpp \
../Startup_Code/system.cpp \
../Startup_Code/vectors.cpp 

S_UPPER_SRCS += \
../Startup_Code/startup_ARMLtdGCC.S 

C_SRCS += \
../Startup_Code/newlib_stubs.c \
../Startup_Code/security.c 

C_DEPS += \
./Startup_Code/newlib_stubs.d \
./Startup_Code/security.d 

OBJS += \
./Startup_Code/console.o \
./Startup_Code/mcg.o \
./Startup_Code/newlib_stubs.o \
./Startup_Code/rtc.o \
./Startup_Code/security.o \
./Startup_Code/startup_ARMLtdGCC.o \
./Startup_Code/system-gcc.o \
./Startup_Code/system.o \
./Startup_Code/vectors.o 

CPP_DEPS += \
./Startup_Code/console.d \
./Startup_Code/mcg.d \
./Startup_Code/rtc.d \
./Startup_Code/system-gcc.d \
./Startup_Code/system.d \
./Startup_Code/vectors.d 


# Each subdirectory must supply rules for building sources it contributes
Startup_Code/%.o: ../Startup_Code/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM C++ Compiler'
	arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb -g3 -O0 -ffunction-sections -fdata-sections -fno-rtti -finline-limit=100 -Wall -Wextra -DDEBUG_BUILD -D__CORTEX_M4 -D__CMSIS_RTOS -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/Sources" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/Project_Headers" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/cmsis" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/cmsis/INC" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/cmsis/SRC" -fno-exceptions -std=gnu++11 -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)"  -o "$@" $<
	@echo 'Finished building: $<'
	@echo ' '

Startup_Code/%.o: ../Startup_Code/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -g3 -O0 -ffunction-sections -fdata-sections -finline-limit=100 -Wall -Wextra -DDEBUG_BUILD -D__CORTEX_M4 -D__CMSIS_RTOS -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/Sources" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/Project_Headers" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/cmsis" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/cmsis/INC" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/cmsis/SRC" -std=c11 -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)"  -o "$@" $<
	@echo 'Finished building: $<'
	@echo ' '

Startup_Code/%.o: ../Startup_Code/%.S
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Assembler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -g3 -O0 -ffunction-sections -fdata-sections -finline-limit=100 -Wall -Wextra -x assembler-with-cpp -DDEBUG_BUILD -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/Project_Headers" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/cmsis" -Wall -Wextra -c -fmessage-length=0  -o "$@" $<
	@echo 'Finished building: $<'
	@echo ' '


