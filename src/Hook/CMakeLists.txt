add_library(VrHook SHARED
    dllmain.cpp
    DX11/DX11Hook.cpp
    DX11/DX11Hook.h
    VR/OpenXRManager.cpp
    VR/OpenXRManager.h
)

target_include_directories(VrHook PRIVATE
    ${CMAKE_SOURCE_DIR}/src/Common
)

target_link_libraries(VrHook PRIVATE
    minhook
    OpenXR::openxr_loader
)

# DX11 libs (Windows)
if(WIN32)
    target_link_libraries(VrHook PRIVATE
        d3d11
        dxgi
        d3dcompiler
    )
endif()
