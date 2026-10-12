# Notes on porting WebKit to amigoid systems

## Prerequisites / SDK

* GCC - I am using GCC11, GCC9+ work for sure. Anything older and you are in unknown territory (morphos branch might work, morphos_3.32.1 likely would not)
* GCC shall have **good** C++ support including threading
  * Make sure examples from https://en.cppreference.com/w/cpp/thread/call_once https://www.cplusplus.com/reference/condition_variable/condition_variable/ work OK, if not, you're in trouble - either you fix those or you'll need to implement your own primitives in WTF instead of using linux/posix ones
* GCC threading normally requires a compliant pthreads library and fast TLS (thread-local storage) available in the kernel. If you're trying to do TLS just inside the pthreads implementation instead, your performance WILL suffer badly
* cmake - using 3.16.2 myself
* GNU make
* Cross-compile environment. You will want 12 or more threads **and** 1GB RAM per each thread you use for compilation (will fail otherwise)

## First Steps

* Start with a native Linux build of jsc (there's a jscore-native makefile rule in the main Makefile on the MorphOS branch)
* Once built, run jsc base tests, make sure they pass
* Cross-compile jsc
  * you'll need target jpeg, png, xslt, sqlite, xml2, pthreads, recentish libICU (depending on branch selected)
  * you'll need to **port** JavaScriptCore and WTF; JSC changes should be minimal, WTF is where actual porting begins
  * first, prepare cmake/Options**platform**.cmake file. use OptionsMorphOS.cmake as reference
  * for jsc, most features may be disabled (recommended). keep anything OFF in OptionsMorphOS as OFF. curl, cairo, hyphen, webp, harfbuzz should be OK to disable at this stage, drag support, content extensions off, whatever is disabled for MORPHOS_MINIMAL should be off
  * prepare wtf/Platform**platform**.cmake, use PlatformMorphOS.cmake as reference, whatever morphos files were added you'll likely need to copy
  * try to get past cmake stage :)
 * Once built, **run** the jsc tests. Those will expose many potential issues with threading / compiler suite
