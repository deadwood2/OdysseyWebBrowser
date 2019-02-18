list(APPEND WTF_SOURCES
    mui/MainThreadMUI.cpp
)
if (${WTF_OS_AROS})
list(APPEND WTF_SOURCES
    mui/execallocator.cpp
    OSAllocatorAROS.cpp
)
elif (${WTF_OS_AMIGAOS4})
list(APPEND WTF_SOURCES
    OSAllocatorMorphOS.cpp
)
elif (${WTF_OS_MORPHOS})
list(APPEND WTF_SOURCES
    OSAllocatorMorphOS.cpp
)
endif()
list(APPEND WTF_SOURCES
    ThreadingPthreads.cpp
    ThreadIdentifierDataPthreads.cpp
)
