# Docker file to create the latest Homegear 0.6 Docker image
FROM homegear/base:raspbian-wheezy
MAINTAINER Sathya Laufer
ENTRYPOINT ["/bin/bash", "-c"]
ADD https://homegear.eu/downloads/nightlies/homegear_current_raspbian7_armhf.deb /
RUN dpkg -i /homegear_current_raspbian7_armhf.deb
RUN rm homegear_current_raspbian7_armhf.deb