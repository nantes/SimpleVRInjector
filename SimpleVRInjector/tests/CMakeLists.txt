add_executable(DX11TestApp WIN32 DummyDX11App.cpp)
if(WIN32)
    target_link_libraries(DX11TestApp d3d11 d3dcompiler)
endif()
