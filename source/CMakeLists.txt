set(TARGET 3Dandelion)

add_executable(${TARGET}
    main.cpp
    window.h
    window.cpp
)

target_link_libraries(${TARGET}
    dxgi.lib
    d3d12.lib
)
