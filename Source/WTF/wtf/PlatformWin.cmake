list(APPEND WTF_PUBLIC_HEADERS
    text/win/WCharStringExtras.h

    win/GDIObject.h
    win/SoftLinking.h
    win/Win32Handle.h
)

list(APPEND WTF_SOURCES
    text/win/TextBreakIteratorInternalICUWin.cpp

    win/CPUTimeWin.cpp
    win/LanguageWin.cpp
    win/MainThreadWin.cpp
    win/MemoryFootprintWin.cpp
    win/MemoryPressureHandlerWin.cpp
    win/RunLoopWin.cpp
    win/WorkQueueWin.cpp
)

if (USE_CF)
    list(APPEND WTF_PUBLIC_HEADERS
        cf/TypeCastsCF.h

        text/cf/TextBreakIteratorCF.h
    )
    list(APPEND WTF_SOURCES
        text/cf/AtomicStringImplCF.cpp
        text/cf/StringCF.cpp
        text/cf/StringImplCF.cpp
        text/cf/StringViewCF.cpp
    )

    list(APPEND WTF_LIBRARIES ${COREFOUNDATION_LIBRARY})
endif ()

set(WTF_OUTPUT_NAME WTF${DEBUG_SUFFIX})
