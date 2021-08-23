set(TARGET 3Dandelion)

add_executable(${TARGET}
    main.cpp
    window.h
    window.cpp
)

target_include_directories(${TARGET}
    PUBLIC
        ${EXTERNAL_DIR}/directx-headers/include/directx
)

target_link_libraries(${TARGET}
    PRIVATE
        dxgi.lib
        d3d12.lib
)
