################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../XMLRPC/Device.cpp \
../XMLRPC/Devices.cpp \
../XMLRPC/LogicalParameter.cpp \
../XMLRPC/PhysicalParameter.cpp \
../XMLRPC/RPCVariable.cpp \
../XMLRPC/Server.cpp 

OBJS += \
./XMLRPC/Device.o \
./XMLRPC/Devices.o \
./XMLRPC/LogicalParameter.o \
./XMLRPC/PhysicalParameter.o \
./XMLRPC/RPCVariable.o \
./XMLRPC/Server.o 

CPP_DEPS += \
./XMLRPC/Device.d \
./XMLRPC/Devices.d \
./XMLRPC/LogicalParameter.d \
./XMLRPC/PhysicalParameter.d \
./XMLRPC/RPCVariable.d \
./XMLRPC/Server.d 


# Each subdirectory must supply rules for building sources it contributes
XMLRPC/%.o: ../XMLRPC/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


