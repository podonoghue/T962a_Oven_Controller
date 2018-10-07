################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Sources/EditProfile.cpp \
../Sources/RemoteInterface.cpp \
../Sources/SolderProfile.cpp \
../Sources/StepResponse.cpp \
../Sources/cdc.cpp \
../Sources/configure.cpp \
../Sources/copyProfile.cpp \
../Sources/delay.cpp \
../Sources/fonts.cpp \
../Sources/ftfl.cpp \
../Sources/hardware.cpp \
../Sources/i2c.cpp \
../Sources/lcd_st7920.cpp \
../Sources/main.cpp \
../Sources/mainMenu.cpp \
../Sources/manageProfiles.cpp \
../Sources/messageBox.cpp \
../Sources/plotting.cpp \
../Sources/reporter.cpp \
../Sources/runProfile.cpp \
../Sources/settings.cpp \
../Sources/spi.cpp \
../Sources/usb.cpp \
../Sources/usb_implementation_cdc.cpp \
../Sources/usbdmError.cpp 

C_SRCS += \
../Sources/RTX_Conf_CM.c 

C_DEPS += \
./Sources/RTX_Conf_CM.d 

OBJS += \
./Sources/EditProfile.o \
./Sources/RTX_Conf_CM.o \
./Sources/RemoteInterface.o \
./Sources/SolderProfile.o \
./Sources/StepResponse.o \
./Sources/cdc.o \
./Sources/configure.o \
./Sources/copyProfile.o \
./Sources/delay.o \
./Sources/fonts.o \
./Sources/ftfl.o \
./Sources/hardware.o \
./Sources/i2c.o \
./Sources/lcd_st7920.o \
./Sources/main.o \
./Sources/mainMenu.o \
./Sources/manageProfiles.o \
./Sources/messageBox.o \
./Sources/plotting.o \
./Sources/reporter.o \
./Sources/runProfile.o \
./Sources/settings.o \
./Sources/spi.o \
./Sources/usb.o \
./Sources/usb_implementation_cdc.o \
./Sources/usbdmError.o 

CPP_DEPS += \
./Sources/EditProfile.d \
./Sources/RemoteInterface.d \
./Sources/SolderProfile.d \
./Sources/StepResponse.d \
./Sources/cdc.d \
./Sources/configure.d \
./Sources/copyProfile.d \
./Sources/delay.d \
./Sources/fonts.d \
./Sources/ftfl.d \
./Sources/hardware.d \
./Sources/i2c.d \
./Sources/lcd_st7920.d \
./Sources/main.d \
./Sources/mainMenu.d \
./Sources/manageProfiles.d \
./Sources/messageBox.d \
./Sources/plotting.d \
./Sources/reporter.d \
./Sources/runProfile.d \
./Sources/settings.d \
./Sources/spi.d \
./Sources/usb.d \
./Sources/usb_implementation_cdc.d \
./Sources/usbdmError.d 


# Each subdirectory must supply rules for building sources it contributes
Sources/%.o: ../Sources/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: ARM C++ Compiler'
	arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb -g3 -O0 -ffunction-sections -fdata-sections -fno-rtti -finline-limit=100 -Wall -Wextra -DDEBUG_BUILD -D__CORTEX_M4 -D__CMSIS_RTOS -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/Sources" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/Project_Headers" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/cmsis" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/cmsis/INC" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/cmsis/SRC" -fno-exceptions -std=gnu++11 -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)"  -o "$@" $<
	@echo 'Finished building: $<'
	@echo ' '

Sources/%.o: ../Sources/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -g3 -O0 -ffunction-sections -fdata-sections -finline-limit=100 -Wall -Wextra -DDEBUG_BUILD -D__CORTEX_M4 -D__CMSIS_RTOS -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/Sources" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/Project_Headers" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/cmsis" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/cmsis/INC" -I"C:/Users/podonoghue/Documents/Development/T962a_Oven_Controller/Software/SMT_Oven_stubs/cmsis/SRC" -std=c11 -c -fmessage-length=0 -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)"  -o "$@" $<
	@echo 'Finished building: $<'
	@echo ' '


