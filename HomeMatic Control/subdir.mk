################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
./BidCoSMessage.cpp \
./BidCoSMessages.cpp \
./BidCoSPacket.cpp \
./BidCoSQueue.cpp \
./Cul.cpp \
./Database.cpp \
./Exception.cpp \
./GD.cpp \
./HM-CC-TC.cpp \
./HM-CC-VD.cpp \
./HM-RC-Sec3-B.cpp \
./HM-SD.cpp \
./HomeMaticDevice.cpp \
./HomeMaticDevices.cpp \
./Log.cpp \
./Peer.cpp \
./main.cpp 

OBJS += \
./BidCoSMessage.o \
./BidCoSMessages.o \
./BidCoSPacket.o \
./BidCoSQueue.o \
./Cul.o \
./Database.o \
./Exception.o \
./GD.o \
./HM-CC-TC.o \
./HM-CC-VD.o \
./HM-RC-Sec3-B.o \
./HM-SD.o \
./HomeMaticDevice.o \
./HomeMaticDevices.o \
./Log.o \
./Peer.o \
./main.o 

CPP_DEPS += \
./BidCoSMessage.d \
./BidCoSMessages.d \
./BidCoSPacket.d \
./BidCoSQueue.d \
./Cul.d \
./Database.d \
./Exception.d \
./GD.d \
./HM-CC-TC.d \
./HM-CC-VD.d \
./HM-RC-Sec3-B.d \
./HM-SD.d \
./HomeMaticDevice.d \
./HomeMaticDevices.d \
./Log.d \
./Peer.d \
./main.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ./%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_REENTRANT -I/usr/lib/pkgconfig/../../include/mono-2.0 -O3 -Wall -c -fmessage-length=0 -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


