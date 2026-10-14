

# OWB (WebKitLegacy/mui/WebCoreLeaks) stops the JIT worklist at exit; export
# the worklist header and the internal headers it includes.
list(APPEND JavaScriptCore_PRIVATE_FRAMEWORK_HEADERS
    heap/ReleaseHeapAccessScope.h
    jit/JITCompilationKey.h
    jit/JITPlan.h
    jit/JITPlanStage.h
    jit/JITWorklist.h
    jit/JITWorklistThread.h
)
