add_subdirectory(directx-headers)
set_target_properties(DirectX-Guids PROPERTIES FOLDER "External/DirectX")

add_subdirectory(glm)
set_target_properties(uninstall PROPERTIES FOLDER "External/GLM")
