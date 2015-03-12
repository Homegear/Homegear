# Docker file to create the latest Homegear 0.6 Docker image
FROM homegear/raspbian:wheezy
MAINTAINER Sathya Laufer
RUN wget http://homegear.eu/packages/Release.key
RUN apt-key add Release.key
RUN rm Release.key
RUN echo "deb http://homegear.eu/packages/Raspbian/ wheezy/" >> /etc/apt/sources.list.d/homegear.list
RUN apt-get update
RUN apt-get install libphp5-embed php5-cli
RUN wget http://homegear.eu/downloads/nightlies/homegear_current_raspbian7_armhf.deb
RUN dpkg -i homegear_current_raspbian7_armhf.deb
RUN rm homegear_current_raspbian7_armhf.deb
RUN apt-get clean
RUN rm -Rf /var/lib/apt/lists/*
