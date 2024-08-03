list(APPEND WebCore_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/platform/graphics/texmap"
)
list(APPEND WebCore_SOURCES
    platform/graphics/texmap/BitmapTexture.cpp
    platform/graphics/texmap/BitmapTexturePool.cpp
    platform/graphics/texmap/GraphicsLayerTextureMapper.cpp
    platform/graphics/texmap/TextureMapper.cpp
    platform/graphics/texmap/TextureMapperAnimation.cpp
    platform/graphics/texmap/TextureMapperBackingStore.cpp
    platform/graphics/texmap/TextureMapperFPSCounter.cpp
    platform/graphics/texmap/TextureMapperLayer.cpp
    platform/graphics/texmap/TextureMapperSurfaceBackingStore.cpp
    platform/graphics/texmap/TextureMapperTile.cpp
    platform/graphics/texmap/TextureMapperTiledBackingStore.cpp
)

if (USE_TEXTURE_MAPPER_GL)
    list(APPEND WebCore_SOURCES
        platform/graphics/texmap/BitmapTextureGL.cpp
        platform/graphics/texmap/ClipStack.cpp
        platform/graphics/texmap/TextureMapperGL.cpp
        platform/graphics/texmap/TextureMapperShaderProgram.cpp
    )
endif ()

if (USE_COORDINATED_GRAPHICS)
    list(APPEND WebCore_INCLUDE_DIRECTORIES
        "${WEBCORE_DIR}/page/scrolling/coordinatedgraphics"
        "${WEBCORE_DIR}/platform/graphics/texmap/coordinated"
    )
    list(APPEND WebCore_SOURCES
        page/scrolling/coordinatedgraphics/ScrollingCoordinatorCoordinatedGraphics.cpp
        page/scrolling/coordinatedgraphics/ScrollingStateNodeCoordinatedGraphics.cpp

        platform/graphics/texmap/coordinated/CoordinatedGraphicsLayer.cpp
        platform/graphics/texmap/coordinated/CoordinatedImageBacking.cpp
        platform/graphics/texmap/coordinated/CoordinatedSurface.cpp
        platform/graphics/texmap/coordinated/Tile.cpp
        platform/graphics/texmap/coordinated/TiledBackingStore.cpp
    )
endif ()

if (ENABLE_THREADED_COMPOSITOR)
    list(APPEND WebCore_SOURCES
        platform/graphics/texmap/TextureMapperPlatformLayerBuffer.cpp
        platform/graphics/texmap/TextureMapperPlatformLayerProxy.cpp
    )
endif ()
