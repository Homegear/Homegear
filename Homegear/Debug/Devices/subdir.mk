################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Devices/HM-CC-TC.cpp \
../Devices/HM-CC-VD.cpp \
../Devices/HM-SD.cpp \
../Devices/HomeMaticCentral.cpp 

OBJS += \
./Devices/HM-CC-TC.o \
./Devices/HM-CC-VD.o \
./Devices/HM-SD.o \
./Devices/HomeMaticCentral.o 

CPP_DEPS += \
./Devices/HM-CC-TC.d \
./Devices/HM-CC-VD.d \
./Devices/HM-SD.d \
./Devices/HomeMaticCentral.d 


# Each subdirectory must supply rules for building sources it contributes
Devices/%.o: ../Devices/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


