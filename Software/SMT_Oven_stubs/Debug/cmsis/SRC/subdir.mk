################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../cmsis/SRC/HAL_CM.c \
../cmsis/SRC/rt_CMSIS.c \
../cmsis/SRC/rt_Event.c \
../cmsis/SRC/rt_List.c \
../cmsis/SRC/rt_Mailbox.c \
../cmsis/SRC/rt_MemBox.c \
../cmsis/SRC/rt_Memory.c \
../cmsis/SRC/rt_Mutex.c \
../cmsis/SRC/rt_Robin.c \
../cmsis/SRC/rt_Semaphore.c \
../cmsis/SRC/rt_System.c \
../cmsis/SRC/rt_Task.c \
../cmsis/SRC/rt_Time.c \
../cmsis/SRC/rt_Timer.c 

C_DEPS += \
./cmsis/SRC/HAL_CM.d \
./cmsis/SRC/rt_CMSIS.d \
./cmsis/SRC/rt_Event.d \
./cmsis/SRC/rt_List.d \
./cmsis/SRC/rt_Mailbox.d \
./cmsis/SRC/rt_MemBox.d \
./cmsis/SRC/rt_Memory.d \
./cmsis/SRC/rt_Mutex.d \
./cmsis/SRC/rt_Robin.d \
./cmsis/SRC/rt_Semaphore.d \
./cmsis/SRC/rt_System.d \
./cmsis/SRC/rt_Task.d \
./cmsis/SRC/rt_Time.d \
./cmsis/SRC/rt_Timer.d 

OBJS += \
./cmsis/SRC/HAL_CM.o \
./cmsis/SRC/rt_CMSIS.o \
./cmsis/SRC/rt_Event.o \
./cmsis/SRC/rt_List.o \
./cmsis/SRC/rt_Mailbox.o \
./cmsis/SRC/rt_MemBox.o \
./cmsis/SRC/rt_Memory.o \
./cmsis/SRC/rt_Mutex.o \
./cmsis/SRC/rt_Robin.o \
./cmsis/SRC/rt_Semaphore.o \
./cmsis/SRC/rt_System.o \
./cmsis/SRC/rt_Task.o \
./cmsis/SRC/rt_Time.o \
./cmsis/SRC/rt_Timer.o 


# Each subdirectory must supply rules for building sources it contributes
cmsis/SRC/%.o: ../cmsis/SRC/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -g3 -O0 -ffunction-sections -fdata-sections -finline-limit=100 -Wall -Wextra -DDEBUG_BUILD -D__CORTEX_M4 -D__CMSIS_RTOS -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/Sources" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/Project_Headers" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/cmsis" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/cmsis/INC" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/cmsis/SRC" -std=c11 -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)"  -o "$@" $<
	@echo 'Finished building: $<'
	@echo ' '


