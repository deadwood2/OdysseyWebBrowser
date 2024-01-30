# this one is important
SET(CMAKE_SYSTEM_NAME Generic)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
SET(CMAKE_CROSSCOMPILING ON)

# options
SET(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "Flags used by the C compiler during RELEASE builds.")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "Flags used by the CXX compiler during RELEASE builds.")

SET(CMAKE_C_FLAGS_RELWITHDEBINFO "-O1 -gdwarf -g1 -DNDEBUG -fno-exceptions -fno-strict-aliasing" CACHE STRING "Flags used by the C compiler during RELWITHDEBINFO builds.")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O1 -gdwarf -g1 -DNDEBUG -fno-exceptions -fno-strict-aliasing -fno-rtti" CACHE STRING "Flags used by the CXX compiler during RELWITHDEBINFO builds.")

SET(CMAKE_MODULE_LINKER_FLAGS "")

# where is the target environment
SET(CMAKE_FIND_ROOT_PATH  "")

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
