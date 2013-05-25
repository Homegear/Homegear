################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../XMLRPC/Device.cpp \
../XMLRPC/LogicalParameter.cpp \
../XMLRPC/PhysicalParameter.cpp \
../XMLRPC/Server.cpp 

OBJS += \
./XMLRPC/Device.o \
./XMLRPC/LogicalParameter.o \
./XMLRPC/PhysicalParameter.o \
./XMLRPC/Server.o 

CPP_DEPS += \
./XMLRPC/Device.d \
./XMLRPC/LogicalParameter.d \
./XMLRPC/PhysicalParameter.d \
./XMLRPC/Server.d 


# Each subdirectory must supply rules for building sources it contributes
XMLRPC/%.o: ../XMLRPC/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/local/include -O3 -Wall -c -fmessage-length=0 -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


