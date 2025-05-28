list(APPEND PAL_PUBLIC_HEADERS
    crypto/gcrypt/Handle.h
    crypto/gcrypt/Initialization.h
    crypto/gcrypt/Utilities.h
)

list(APPEND PAL_SOURCES
    crypto/openssl/CryptoDigestOpenSSL.cpp
#    crypto/gcrypt/CryptoDigestGCrypt.cpp

    system/ClockGeneric.cpp
    system/morphos/SoundMorphOS.cpp

    text/KillRing.cpp

    unix/LoggingUnix.cpp
)

if (ENABLE_WEB_CRYPTO)
    list(APPEND PAL_PUBLIC_HEADERS
        crypto/tasn1/Utilities.h
    )

    list(APPEND PAL_SOURCES
        crypto/tasn1/Utilities.cpp
    )
endif ()

list(APPEND PAL_SYSTEM_INCLUDE_DIRECTORIES ${OPENSSL_INCLUDE_DIR})

list(APPEND PAL_LIBRARIES ${OPENSSL_CRYPTO_LIBRARY})
