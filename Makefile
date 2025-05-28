ROOTPATH:=$(abspath ../../../)
LIB:=$(ROOTPATH)/lib
GEN:=$(ROOTPATH)/gen/host/libnix

PKG_ICU:=$(LIB)/libicu67/instdir/lib/pkgconfig/
PKG_SQLITE:=$(LIB)/sqlite/instdir/lib/pkgconfig/
PKG_FONTCONFIG:=$(ROOTPATH)/morphoswb/libs/fontconfig/MorphOS/
PKG:=$(PKG_ICU):$(PKG_SQLITE)

DEBIAN_PKG:=libicu-dev ruby-dev clang-7
NATIVE_GCC:=/home/jaca/gcc7/inst/bin/x86_64-pc-linux-gnu-

OBJC:=$(ROOTPATH)/morphoswb/classes/frameworks/includes/

CMAKE = $(abspath cmake-3.16.2/bin)

all:

configure-native:
	rm -rf build
	mkdir build
	(cd build && PATH=~/cmake-3.10.3-Linux-x86_64/bin/:${PATH} \
		cmake -DCMAKE_MODULE_PATH=$(realpath Source/cmake) \
		-DCMAKE_BUILD_TYPE=Release -DPORT=JSCOnly -DUSE_SYSTEM_MALLOC=YES -DCMAKE_CXX_FLAGS="-O2 -fPIC" -DCMAKE_C_FLAGS="-O2 -fPIC" \
		-DCMAKE_C_COMPILER=$(NATIVE_GCC)gcc -DCMAKE_CXX_COMPILER=$(NATIVE_GCC)g++ \
		$(realpath ./))

jscore-native:
	rm -rf WebKitBuild build
	mkdir build
	(cd build && PATH=$(CMAKE):${PATH} \
		$(realpath Tools/Scripts/run-javascriptcore-tests) --jsc-only --no-flt-jit \
		--cmakeargs='-DCMAKE_MODULE_PATH=$(realpath Source/cmake) -DJAVASCRIPTCORE_DIR=$(realpath Source/JavaScriptCore) \
                -DCMAKE_BUILD_TYPE=Release -DPORT=JSCOnly -DUSE_SYSTEM_MALLOC=YES -DCMAKE_CXX_FLAGS="-O2 -fPIC" -DCMAKE_C_FLAGS="-O2 -fPIC" \
                -DCMAKE_C_COMPILER=$(NATIVE_GCC)gcc -DCMAKE_CXX_COMPILER=$(NATIVE_GCC)g++')
	cp -a Source/JavaScriptCore/API/tests/testapiScripts ~/morphos/morphoswb/apps/webkitty/WebKitBuild/Release/Source/JavaScriptCore/shell/
	Tools/Scripts/run-javascriptcore-tests --root WebKitBuild/Release/Source/JavaScriptCore/shell/ --no-jsc-stress --no-jit-stress-test

jscore-morphos: morphos.cmake
	rm -rf WebKitBuild cross-build
	mkdir -p cross-build WebKitBuild/Release/bin
	(cd cross-build && PKG_CONFIG_PATH=$(PKG) PATH=$(CMAKE):${PATH} \
		$(realpath Tools/Scripts/run-javascriptcore-tests) --jsc-only --no-flt-jit \
		--cmakeargs='-DCMAKE_CROSSCOMPILING=ON -DCMAKE_TOOLCHAIN_FILE=$(realpath morphos.cmake) -DCMAKE_MODULE_PATH=$(realpath Source/cmake) \
		-DJAVASCRIPTCORE_DIR=$(realpath Source/JavaScriptCore) -DBUILD_SHARED_LIBS=NO \
		-DJPEG_LIBRARY=$(LIB)/libjpeg -DJPEG_INCLUDE_DIR=$(LIB)/libjpeg \
		-DLIBXML2_LIBRARY=$(LIB)/libxml2/instdir/lib -DLIBXML2_INCLUDE_DIR=$(LIB)/libxml2/instdir/include/libxml2 \
		-DPNG_LIBRARY=$(GEN)/libpng16/lib/ -DPNG_INCLUDE_DIR=$(GEN)/libpng16/include \
		-DLIBXSLT_LIBRARIES=$(LIB)/libxslt/instdir/lib -DLIBXSLT_INCLUDE_DIR=$(LIB)/libxslt/instdir/include \
		-DSQLITE_LIBRARIES=$(LIB)/sqlite/instdir/lib -DSQLITE_INCLUDE_DIR=$(LIB)/sqlite/instdir/include \
                -DCMAKE_BUILD_TYPE=Release -DPORT=JSCOnly -DUSE_SYSTEM_MALLOC=YES \
		-DCMAKE_FIND_LIBRARY_SUFFIXES=".a" ')
	cp -a Source/JavaScriptCore/API/tests/testapiScripts ./WebKitBuild/Release/Source/JavaScriptCore/shell/
#	Tools/Scripts/run-javascriptcore-tests --root WebKitBuild/Release/Source/JavaScriptCore/shell/ --no-jsc-stress --no-jit-stress-test

jscore-pack:
	ppc-morphos-strip ./WebKitBuild/Release/Source/JavaScriptCore/shell/jsc
	ppc-morphos-strip ./WebKitBuild/Release/Source/JavaScriptCore/shell/testRegExp
	ppc-morphos-strip ./WebKitBuild/Release/Source/JavaScriptCore/shell/testair
	ppc-morphos-strip ./WebKitBuild/Release/Source/JavaScriptCore/shell/testapi
	ppc-morphos-strip ./WebKitBuild/Release/Source/JavaScriptCore/shell/testb3
	ppc-morphos-strip ./WebKitBuild/Release/Source/JavaScriptCore/shell/testdfg
	ppc-morphos-strip ./WebKitBuild/Release/Source/JavaScriptCore/shell/testmasm
	tar cJf testsuite.tar.xz Tools ./WebKitBuild/Release/Source/JavaScriptCore/shell/jsc \
		./WebKitBuild/Release/Source/JavaScriptCore/shell/testRegExp \
		./WebKitBuild/Release/Source/JavaScriptCore/shell/testair \
		./WebKitBuild/Release/Source/JavaScriptCore/shell/testapi \
		./WebKitBuild/Release/Source/JavaScriptCore/shell/testb3 \
		./WebKitBuild/Release/Source/JavaScriptCore/shell/testdfg \
		./WebKitBuild/Release/Source/JavaScriptCore/shell/testmasm \
		 JSTests LayoutTests PerformanceTests

configure: morphos.cmake link.sh CMakeLists.txt Dummy/libdummy.a ffmpeg/.buildstamp
	rm -rf cross-build
	mkdir cross-build
	(cd cross-build && PKG_CONFIG_PATH=$(PKG) PATH=$(CMAKE):${PATH} \
		cmake -DCMAKE_CROSSCOMPILING=ON -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DCMAKE_TOOLCHAIN_FILE=$(realpath morphos.cmake) -DCMAKE_DL_LIBS="syscall" \
		-DBUILD_SHARED_LIBS=NO -DPORT=MorphOS -DENABLE_WEBCORE=1 -DENABLE_WEBKIT_LEGACY=1 -DLOG_DISABLED=0 -DMORPHOS_MINIMAL=0 -DROOTPATH="$(ROOTPATH)" \
		-DJPEG_LIBRARY=$(LIB)/libjpeg/libjpeg.a \
		-DJPEG_INCLUDE_DIR=$(LIB)/libjpeg \
		-DLIBXML2_LIBRARY=$(LIB)/libxml2/instdir/lib/libxml2.a \
		-DLIBXML2_INCLUDE_DIR="$(LIB)/libxml2/instdir/include/libxml2/" \
		-DPNG_LIBRARIES=$(GEN)/libpng16/lib/libpng16.a \
		-DPNG_PNG_INCLUDE_DIR=$(GEN)/libpng16/include/libpng16/ \
		-DPNG_INCLUDE_DIRS=$(GEN)/libpng16/include/libpng16/ \
		-DLIBXSLT_LIBRARIES=$(LIB)/libxslt/instdir/lib/libxslt.a \
		-DLIBXSLT_INCLUDE_DIR=$(LIB)/libxslt/instdir/include \
		-DSQLITE_LIBRARIES=$(LIB)/sqlite/instdir/lib/libsqlite3.a \
		-DSQLITE_INCLUDE_DIR=$(LIB)/sqlite/instdir/include \
		-DSQLite3_LIBRARY=$(LIB)/sqlite/instdir/include \
		-DSQLite3_INCLUDE_DIR=$(LIB)/sqlite/instdir/include \
		-DCAIRO_INCLUDE_DIRS=$(ROOTPATH)/morphoswb/libs/cairo/MorphOS/os-include/cairo \
		-DCAIRO_LIBRARIES="$(ROOTPATH)/morphoswb/libs/cairo/MorphOS/lib/libnix/libcairo.a" \
		-DCairo_INCLUDE_DIR=$(ROOTPATH)/morphoswb/libs/cairo/MorphOS/os-include/cairo \
		-DCairo_LIBRARY="$(ROOTPATH)/morphoswb/libs/cairo/MorphOS/lib/libnix/libcairo.a" \
		-DHarfBuzz_INCLUDE_DIR="$(realpath Dummy)"\
		-DHarfBuzz_LIBRARY=$(GEN)/lib/libnghttp2.a \
		-DICU_ROOT="$(LIB)/libicu67/instdir/" \
		-DICU_UC_LIBRARY_RELEASE="$(LIB)/libicu67/instdir/lib/libicuuc.a" \
		-DICU_DATA_LIBRARY_RELEASE="$(LIB)/libicu67/instdir/lib/libicudata.a" \
		-DICU_I18N_LIBRARY_RELEASE="$(LIB)/libicu67/instdir/lib/libicui18n.a" \
		-DHarfBuzz_ICU_LIBRARY="$(realpath Dummy)/libdummy.a" \
		-DFREETYPE_INCLUDE_DIRS="$(ROOTPATH)/morphoswb/libs/freetype/include" \
		-DFREETYPE_LIBRARY="$(ROOTPATH)/morphoswb/libs/freetype/library/lib/libfreetype.a" \
		-DFontconfig_LIBRARY="$(ROOTPATH)/morphoswb/libs/fontconfig/MorphOS/libfontconfig-glue.a" \
		-DFontconfig_INCLUDE_DIR="$(ROOTPATH)/morphoswb/libs/fontconfig" \
		-DOpenJPEG_INCLUDE_DIR="$(GEN)/include/openjpeg-2.5" \
		-DWebP_INCLUDE_DIR="$(GEN)/include" -DWebP_LIBRARY="$(GEN)/lib/libwebp.a" -DWebP_DEMUX_LIBRARY="$(GEN)/lib/libwebpdemux.a"\
		-DAVFORMAT_LIBRARY="ffmpeg/instdir/lib/libavformat.a" -DAVFORMAT_INCLUDE_DIR="$(realpath ffmpeg/instdir/include)" \
		-DAVCODEC_LIBRARY="ffmpeg/instdir/lib/libavcodec.a" -DAVCODEC_INCLUDE_DIR="$(realpath ffmpeg/instdir/include)" \
		-DAVUTIL_LIBRARY="ffmpeg/instdir/lib/libavutil.a" -DAVUTIL_INCLUDE_DIR="$(realpath ffmpeg/instdir/include)" \
		-DSWSCALE_LIBRARY="ffmpeg/instdir/lib/libswscale.a" -DSWSCALE_INCLUDE_DIR="$(realpath ffmpeg/instdir/include)" \
		-DWOFF2_LIBRARY="$(GEN)/lib/libwoff2dec.a $(GEN)/lib/libwoff2common.a" -DWOFF2_INCLUDE_DIR="$(GEN)/include/" \
		-DOBJC_INCLUDE="$(OBJC)" \
		-DCMAKE_MODULE_PATH=$(realpath Source/cmake) $(realpath ./))

configure-mini: morphos.cmake link.sh CMakeLists.txt Dummy/libdummy.a ffmpeg/.buildstamp
	rm -rf cross-build-mini
	mkdir cross-build-mini
	(cd cross-build-mini && PKG_CONFIG_PATH=$(PKG) PATH=$(CMAKE):${PATH} \
		cmake -DCMAKE_CROSSCOMPILING=ON -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DCMAKE_TOOLCHAIN_FILE=$(realpath morphos.cmake) -DCMAKE_DL_LIBS="syscall" \
		-DBUILD_SHARED_LIBS=NO -DPORT=MorphOS -DENABLE_WEBCORE=1 -DENABLE_WEBKIT_LEGACY=1 -DLOG_DISABLED=0 -DMORPHOS_MINIMAL=1 -DROOTPATH="$(ROOTPATH)" \
		-DJPEG_LIBRARY=$(LIB)/libjpeg/libjpeg.a \
		-DJPEG_INCLUDE_DIR=$(LIB)/libjpeg \
		-DLIBXML2_LIBRARY=$(LIB)/libxml2/instdir/lib/libxml2.a \
		-DLIBXML2_INCLUDE_DIR="$(LIB)/libxml2/instdir/include/libxml2/" \
		-DPNG_LIBRARIES=$(GEN)/libpng16/lib/libpng16.a \
		-DPNG_PNG_INCLUDE_DIR=$(GEN)/libpng16/include/libpng16/ \
		-DPNG_INCLUDE_DIRS=$(GEN)/libpng16/include/libpng16/ \
		-DLIBXSLT_LIBRARIES=$(LIB)/libxslt/instdir/lib/libxslt.a \
		-DLIBXSLT_INCLUDE_DIR=$(LIB)/libxslt/instdir/include \
		-DSQLITE_LIBRARIES=$(LIB)/sqlite/instdir/lib/libsqlite3.a \
		-DSQLITE_INCLUDE_DIR=$(LIB)/sqlite/instdir/include \
		-DSQLite3_LIBRARY=$(LIB)/sqlite/instdir/include \
		-DSQLite3_INCLUDE_DIR=$(LIB)/sqlite/instdir/include \
		-DCAIRO_INCLUDE_DIRS=$(ROOTPATH)/morphoswb/libs/cairo/MorphOS/os-include/cairo \
		-DCAIRO_LIBRARIES="$(ROOTPATH)/morphoswb/libs/cairo/MorphOS/lib/libnix/libcairo.a" \
		-DCairo_INCLUDE_DIR=$(ROOTPATH)/morphoswb/libs/cairo/MorphOS/os-include/cairo \
		-DCairo_LIBRARY="$(ROOTPATH)/morphoswb/libs/cairo/MorphOS/lib/libnix/libcairo.a" \
		-DHarfBuzz_INCLUDE_DIR="$(realpath Dummy)"\
		-DHarfBuzz_LIBRARY=$(GEN)/lib/libnghttp2.a \
		-DICU_ROOT="$(LIB)/libicu67/instdir/" \
		-DICU_UC_LIBRARY_RELEASE="$(LIB)/libicu67/instdir/lib/libicuuc.a" \
		-DICU_DATA_LIBRARY_RELEASE="$(LIB)/libicu67/instdir/lib/libicudata.a" \
		-DICU_I18N_LIBRARY_RELEASE="$(LIB)/libicu67/instdir/lib/libicui18n.a" \
		-DHarfBuzz_ICU_LIBRARY="$(realpath Dummy)/libdummy.a" \
		-DFREETYPE_INCLUDE_DIRS="$(ROOTPATH)/morphoswb/libs/freetype/include" \
		-DFREETYPE_LIBRARY="$(ROOTPATH)/morphoswb/libs/freetype/library/lib/libfreetype.a" \
		-DFontconfig_LIBRARY="$(ROOTPATH)/morphoswb/libs/fontconfig/MorphOS/libfontconfig-glue.a" \
		-DFontconfig_INCLUDE_DIR="$(ROOTPATH)/morphoswb/libs/fontconfig" \
		-DOpenJPEG_INCLUDE_DIR="$(GEN)/include/openjpeg-2.5" \
		-DWebP_INCLUDE_DIR="$(GEN)/include" -DWebP_LIBRARY="$(GEN)/lib/libwebp.a" -DWebP_DEMUX_LIBRARY="$(GEN)/lib/libwebpdemux.a"\
		-DAVFORMAT_LIBRARY="ffmpeg/instdir/lib/libavformat.a" -DAVFORMAT_INCLUDE_DIR="$(realpath ffmpeg/instdir/include)" \
		-DAVCODEC_LIBRARY="ffmpeg/instdir/lib/libavcodec.a" -DAVCODEC_INCLUDE_DIR="$(realpath ffmpeg/instdir/include)" \
		-DAVUTIL_LIBRARY="ffmpeg/instdir/lib/libavutil.a" -DAVUTIL_INCLUDE_DIR="$(realpath ffmpeg/instdir/include)" \
		-DSWSCALE_LIBRARY="ffmpeg/instdir/lib/libswscale.a" -DSWSCALE_INCLUDE_DIR="$(realpath ffmpeg/instdir/include)" \
		-DOBJC_INCLUDE="$(OBJC)" \
		-DCMAKE_MODULE_PATH=$(realpath Source/cmake) $(realpath ./))

build:
	(cd cross-build && make -j$(shell nproc))
	echo "Link done"
	ppc-morphos-strip cross-build/Tools/morphos/MiniBrowser.db -o cross-build/Tools/morphos/MiniBrowser
	echo "Stripped binary in cross-build/Tools/morphos/MiniBrowser"

#		-Wdev --debug-output --trace --trace-expand \

build-mini:
	(cd cross-build-mini && make -j$(shell nproc))

cross-build:
	make configure

.build: cross-build build

cross-build-mini:
	make configure-mini

.build-mini: cross-build-mini build-mini

morphos.cmake: morphos.cmake.in
	gcc -xc -E -P -C -o$@ -nostdinc $@.in -D_IN_ROOTPATH=$(ROOTPATH) -D_IN_DUMMYPATH=$(realpath Dummy)

link.sh: link.sh.in
	gcc -xc -E -P -C -o$@ -nostdinc $@.in -D_IN_ROOTPATH=$(ROOTPATH)
	chmod u+x $@

libwebkit.a:
	(cd cross-build/Source/WebKitLegacy && make)

clean:
	rm -rf morphos.cmake cross-build cross-build-mini WebKitBuild build link.sh

install:

install-iso:

source:

sdk:

$(CMAKE):
	rm -rf cmake-3.16.2
	tar xf cmake-3.16.2.tar.gz
	(cd cmake-3.16.2 && ./bootstrap -- -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_USE_OPENSSL=OFF )
	(cd cmake-3.16.2 && make -j$(shell nproc))

Dummy/libdummy.a:
	ppc-morphos-gcc-9 -c -o Dummy/dummy.o Dummy/dummy.c
	ppc-morphos-ar rc Dummy/libdummy.a Dummy/dummy.o
	ppc-morphos-ranlib Dummy/libdummy.a
	cp Dummy/libdummy.a Dummy/libdl.a

ffmpeg/.buildstamp:
	cd ffmpeg && make

miniscp:
	scp cross-build/Tools/morphos/MiniBrowser jaca@192.168.2.5:/Users/jaca

minidump:
	@read -p "Address:" address; \
	ppc-morphos-objdump --demangle --disassemble -l --source cross-build/Tools/morphos/MiniBrowser.db --start-address $$address | less

minirelease:
	rm -rf WebKitty webkitty.tar webkitty.tar.xz webkitty.lha
	mkdir -p WebKitty/MOSSYS/Data/ICU
	cp cross-build/Tools/morphos/MiniBrowser WebKitty/
	cp -a Source/WebCore/Resources WebKitty/Resources
	cp -a $(ROOTPATH)/lib/libicu/instdir/icu/54.2/icudt54b WebKitty/MOSSYS/Data/ICU/icudt54b
#	( cd WebKitty/Resources && wget https://easylist.to/easylist/easylist.txt )
	cp easylist/easylist.dat WebKitty/Resources
	mkdir WebKitty/MiniResources
	cp Tools/morphos/MiniResources/*.png WebKitty/MiniResources
	cp Tools/morphos/MiniResources/MiniBrowser.info WebKitty/
	cp MUSTREAD.txt WebKitty/
	lha ao5 webkitty.lha WebKitty
	rm -rf WebKitty

putrelease: minirelease
	scp webkitty.lha jaca@tunkki.dk:/home/jaca/public_html

LINKFILES := \
	cross-build-mini/lib/libWebKit.a \
	cross-build-mini/lib/libWebCore.a \
	cross-build-mini/lib/libPAL.a \
	cross-build-mini/lib/libJavaScriptCore.a \
	cross-build-mini/lib/libWTF.a \
	$(ROOTPATH)/lib/libxml2/instdir/lib/libxml2.a \
	$(ROOTPATH)/lib/libxslt/instdir/lib/libxslt.a \
	$(ROOTPATH)/lib/sqlite/instdir/lib/libsqlite3.a \
	$(ROOTPATH)/gen/host/libnix/lib/libz.a \
	$(ROOTPATH)/morphoswb/libs/cairo/MorphOS/lib/libnix/libcairo.a \
	$(ROOTPATH)/gen/host/libnix/lib/libcurl.a \
	$(ROOTPATH)/gen/host/libnix/lib/libssl.a \
	$(ROOTPATH)/morphoswb/libs/freetype/library/lib/libfreetype.a \
	$(ROOTPATH)/gen/host/libnix/lib/libnghttp2.a \
	$(ROOTPATH)/lib/libjpeg/libjpeg.a \
	$(ROOTPATH)/gen/host/libnix/lib/libpsl.a \
	$(ROOTPATH)/gen/host/libnix/libpng16/lib/libpng16.a  \
	$(ROOTPATH)/gen/host/libnix/lib/libhyphen.a \
	$(ROOTPATH)/gen/host/libnix/lib/libcrypto.a \
	$(ROOTPATH)/lib/libicu67/instdir/lib/libicui18n.a \
	$(ROOTPATH)/lib/libicu67/instdir/lib/libicuuc.a \
	$(ROOTPATH)/lib/libicu67/instdir/lib/libicudata.a \
	$(ROOTPATH)/lib/libwebp/objects/host-libnix/tmpinstalldir/lib/libwebp.a \
	$(ROOTPATH)/lib/libwebp/objects/host-libnix/tmpinstalldir/lib/libwebpdemux.a \
	$(ROOTPATH)/gen/host/libnix/lib/libopenjp2.a

.PHONY: linkpackage
linkpackage:
	@rm -rf linkpackage
	@mkdir linkpackage
	@for i in $(LINKFILES); \
	do echo -n "ppc-morphos-strip --strip-debug $$i -o linkpackage/">.run.sh; \
	echo $$i | rev | cut -d'/' -f-1 | rev >>.run.sh ; \
	echo "Copying and stripping $$i"; \
	bash ./.run.sh; \
	done
	@rm ./.run.sh
