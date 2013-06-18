################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../RPC/Client.cpp \
../RPC/Device.cpp \
../RPC/Devices.cpp \
../RPC/LogicalParameter.cpp \
../RPC/PhysicalParameter.cpp \
../RPC/RPCClient.cpp \
../RPC/RPCDecoder.cpp \
../RPC/RPCEncoder.cpp \
../RPC/RPCMethod.cpp \
../RPC/RPCMethods.cpp \
../RPC/RPCServer.cpp \
../RPC/RPCVariable.cpp \
../RPC/Server.cpp \
../RPC/XMLRPCDecoder.cpp \
../RPC/XMLRPCEncoder.cpp 

OBJS += \
./RPC/Client.o \
./RPC/Device.o \
./RPC/Devices.o \
./RPC/LogicalParameter.o \
./RPC/PhysicalParameter.o \
./RPC/RPCClient.o \
./RPC/RPCDecoder.o \
./RPC/RPCEncoder.o \
./RPC/RPCMethod.o \
./RPC/RPCMethods.o \
./RPC/RPCServer.o \
./RPC/RPCVariable.o \
./RPC/Server.o \
./RPC/XMLRPCDecoder.o \
./RPC/XMLRPCEncoder.o 

CPP_DEPS += \
./RPC/Client.d \
./RPC/Device.d \
./RPC/Devices.d \
./RPC/LogicalParameter.d \
./RPC/PhysicalParameter.d \
./RPC/RPCClient.d \
./RPC/RPCDecoder.d \
./RPC/RPCEncoder.d \
./RPC/RPCMethod.d \
./RPC/RPCMethods.d \
./RPC/RPCServer.d \
./RPC/RPCVariable.d \
./RPC/Server.d \
./RPC/XMLRPCDecoder.d \
./RPC/XMLRPCEncoder.d 


# Each subdirectory must supply rules for building sources it contributes
RPC/%.o: ../RPC/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	arm-linux-gnueabi-g++-4.7 -O3 -Wall -c -fmessage-length=0 -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


