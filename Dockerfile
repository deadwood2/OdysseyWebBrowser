
FROM jaokim/amigade:20190225_2255
 
RUN apt-get update
RUN apt-get --assume-yes install make cmake bison
#RUN dep-get.py -i
WORKDIR /workdir
