# this one is important
SET(CMAKE_SYSTEM_NAME Generic)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
SET(CMAKE_C_COMPILER ppc-amigaos-gcc)
SET(CMAKE_CXX_COMPILER ppc-amigaos-g++)

# options
SET(CMAKE_CXX_FLAGS "-D__USE_INLINE__")
SET(CMAKE_CXX_FLAGS_DEBUG "-g -gdwarf-3")

SET(CMAKE_C_FLAGS "-D__USE_INLINE__")
SET(CMAKE_C_FLAGS_DEBUG "-g -gdwarf-3")

SET(CMAKE_MODULE_LINKER_FLAGS "")

# where is the target environment
SET(CMAKE_FIND_ROOT_PATH  "")

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
