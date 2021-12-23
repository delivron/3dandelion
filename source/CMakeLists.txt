###############################################################################
# Executable

set(TARGET 3Dandelion)

set(SHADERS
    shaders/main.hlsl
)

add_executable(${TARGET}
    main.cpp

    utils.h
    utils.cpp
    window.h
    window.cpp
    application.h
    application.cpp
    fence.h
    fence.cpp
    swap-chain.h
    swap-chain.cpp
    command-queue.h
    command-queue.cpp
    camera.h
    camera.cpp

    ${SHADERS}
)

foreach(SHADER ${SHADERS})
    set_source_files_properties(${SHADER} PROPERTIES VS_TOOL_OVERRIDE "None")
endforeach()

target_link_libraries(${TARGET}
    PRIVATE
        glm
        dxgi
        d3d12
        d3dcompiler
        DirectX-Guids
        DirectX-Headers
)

target_compile_definitions(${TARGET}
    PRIVATE
        _UNICODE
        UNICODE
        NOMINMAX
        GLM_FORCE_LEFT_HANDED
)

target_compile_features(${TARGET}
    PRIVATE
        cxx_std_17
)

###############################################################################
# Custom Target

set(HELPER_TARGET CopyShaders)

add_custom_target(${HELPER_TARGET}
    COMMAND ${CMAKE_COMMAND}
    -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders
)

add_dependencies(${TARGET} ${HELPER_TARGET})
