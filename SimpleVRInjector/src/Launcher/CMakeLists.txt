add_executable(VrLauncher
    Main.cpp
)

target_link_libraries(VrLauncher PRIVATE
    OpenXR::openxr_loader
)

# Copy the Hook DLL to the same output directory
add_custom_command(TARGET VrLauncher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:VrHook>
    $<TARGET_FILE_DIR:VrLauncher>/VrHook.dll
)
