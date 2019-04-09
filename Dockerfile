# sudo docker build . -t odyssey_build
# sudo docker tag odyssey_build:latest jaokim/aos4-odyssey-build:20190313_2326
# sudo docker push jaokim/aos4-odyssey-build:20190313_2326
FROM jaokim/amigade:20190313_2205
 
RUN apt-get update
RUN apt-get --assume-yes install make cmake bison gperf ruby

#RUN dep-get.py --update 
RUN dep-get.py -i "http://os4depot.net/share/development/library/misc/libz.lha"
RUN dep-get.py -i "http://os4depot.net/share/development/library/misc/libicu.lha"
RUN dep-get.py -i "https://github.com/jaokim/aros-stuff/releases/download/1.0/pthreads.lha"
RUN dep-get.py -i "http://os4depot.net/share/development/library/misc/libcurl.lha"
RUN dep-get.py -i "http://os4depot.net/share/development/library/misc/sqlite.lha"
RUN dep-get.py -i "http://os4depot.net/share/library/xml/libxml2.lha"
RUN dep-get.py -i "http://os4depot.net/share/development/library/graphics/libfreetype.lha"
RUN dep-get.py -i libxslt-1.1.30
RUN dep-get.py -i "http://downloads.sourceforge.net/project/odyssey-os4-dependencies/cairo_lib-1.14.10.tar"
RUN dep-get.py -i "http://os4depot.net/share/development/library/misc/libopenssl.lha"
RUN dep-get.py -i MUI5
RUN dep-get.py -i "http://os4depot.net/share/development/misc/libffmpeg.lha"
RUN dep-get.py -i "http://os4depot.net/share/library/misc/unilibdev.lha"
RUN dep-get.py -i libuuid
RUN dep-get.py -i libfontconfig-2.13.1
RUN dep-get.py -i "http://os4depot.net/share/development/library/graphics/libjpeg.lha" 
RUN dep-get-py -i "http://os4depot.net/share/development/library/graphics/libpng.lha"

RUN apt-get install perl-modules-5.22

WORKDIR /workdir
