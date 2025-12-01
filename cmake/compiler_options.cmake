function(apply_common_compiler_flags target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
                /W4
                /permissive-
                /Zc:__cplusplus
                /MP               # parallel compilation on Windows
                /bigobj           # needed for large projects
        )
    else()
        target_compile_options(${target} PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wshadow
                -Wconversion
                -Wsign-conversion
                -fvisibility=hidden
        )

        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(${target} PRIVATE
                    -Wno-unused-parameter
                    -Wno-unused-variable
                    -Wno-microsoft-cast   # if using clang on windows
            )
        endif()
    endif()

    # Optimizations per config
    target_compile_options(${target} PRIVATE
            $<$<CONFIG:Debug>:-O0 -g3>
            $<$<CONFIG:Release>:-O3 -march=native -flto=auto>
            $<$<CONFIG:RelWithDebInfo>:-O2 -g -march=native>
    )
endfunction()