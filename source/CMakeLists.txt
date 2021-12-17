set(TARGET 3Dandelion)

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
)

target_include_directories(${TARGET}
    PUBLIC
        ${EXTERNAL_DIR}/directx-headers/include/directx
)

target_link_libraries(${TARGET}
    PRIVATE
        glm
        dxgi
        d3d12
        d3dcompiler
)

target_compile_definitions(${TARGET}
    PRIVATE
        _UNICODE
        UNICODE
        NOMINMAX
)

target_compile_features(${TARGET}
    PRIVATE
        cxx_std_17
)

add_custom_command(TARGET ${TARGET}
    PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_CURRENT_SOURCE_DIR}/shaders" "${CMAKE_CURRENT_BINARY_DIR}/shaders")
