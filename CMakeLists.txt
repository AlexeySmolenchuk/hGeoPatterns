cmake_minimum_required(VERSION 3.0.0)
project(hGeoPatterns VERSION 1.0)

# prevent attempt to build right in the project
if ("${PROJECT_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message (FATAL_ERROR "Not allowed to run in-source build!")
endif ()

# fetch required environment variables
set (vars HFS RMANTREE RFHTREE)
foreach (var ${vars})
    if(NOT DEFINED ENV{${var}})
        message(FATAL_ERROR "require " ${var} " environment variable")
    else()
        set(${var} $ENV{${var}})
        message(STATUS "Using: " $ENV{${var}})
    endif ()
endforeach ()

# Houdini and RFH can setup PYTHONPATH for us
set(HOUDINI_PACKAGE_DIR ${RFHTREE}/packages)

include_directories(SYSTEM ${RMANTREE}/include )
link_directories( ${RMANTREE}/lib )

include_directories(SYSTEM ${HFS}/toolkit/include )
link_directories( ${HFS}/dsolib ${HFS}/custom/houdini/dsolib )

add_definitions(-DSIZEOF_VOID_P=8 -D_USE_MATH_DEFINES -DSESI_LITTLE_ENDIAN -DAMD64)
add_definitions(-DNDEBUG)


if (MSVC)
    add_compile_options(-w)
    link_libraries(libprman libpxrcore libstats)
    link_libraries(libGA libGU libGEO libUT libSYS)
else()
    add_definitions(-DLINUX -DUSE_PTHREADS)
    add_compile_options(-W)
    link_libraries(prman pxrcore stats)
    link_libraries(HoudiniGEO HoudiniUT)
endif()

# gather sources
file (GLOB rixplugin_sources "src/*.cpp")
file (GLOB args_files "rixplugins/Args/*.args")
file (GLOB shader_headers "osl/include/*.h")
file (GLOB shader_sources "osl/*.osl")

# compile rixplugins
foreach (_rixpluginame ${rixplugin_sources})
    # cmake_path(GET _rixpluginame STEM name)
    get_filename_component ( name ${_rixpluginame} NAME_WLE )
    add_library( ${name} SHARED ${_rixpluginame})
    if (MSVC)
        set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/rixplugins PREFIX "")
    else()
        set_target_properties(${name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/rixplugins PREFIX "")
    endif()
endforeach ()


# compile OSL shaders
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/shaders)
foreach (oslfile ${shader_sources})
    # cmake_path(GET oslfile STEM name)
    get_filename_component ( name ${oslfile} NAME_WLE )
    set( osofile ${PROJECT_SOURCE_DIR}/shaders/${name}.oso)
    add_custom_command(
        COMMAND ${RMANTREE}/bin/oslc
            -I${PROJECT_SOURCE_DIR}/osl/include
            -o ${osofile}
            ${oslfile}
        OUTPUT ${osofile}
        MAIN_DEPENDENCY ${oslfile}
        DEPENDS ${shader_headers}
    )
    list (APPEND shader_objs ${osofile})
endforeach ()

add_custom_target(OSL
                DEPENDS ${shader_objs}
                SOURCES ${shader_sources} ${shader_headers})


# create HDA
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/otls)
add_custom_command(
    COMMAND  ${CMAKE_COMMAND} -E
        if (MSVC)
            env HOUDINI_PACKAGE_DIR=${HOUDINI_PACKAGE_DIR}
            ${HFS}/bin/hython ${PROJECT_SOURCE_DIR}/python/build_hda.py
        else()
            env PATH="${HFS}/bin:$ENV{PATH}"
            hython ${PROJECT_SOURCE_DIR}/python/build_hda.py
        endif()
        ${PROJECT_SOURCE_DIR}/otls/hGeoPatterns.hda
        ${PROJECT_SOURCE_DIR}/rixplugins
        ${PROJECT_SOURCE_DIR}/shaders
    OUTPUT ${PROJECT_SOURCE_DIR}/otls/hGeoPatterns.hda
    DEPENDS ${args_files} ${shader_objs} ${PROJECT_SOURCE_DIR}/python/build_hda.py
)

add_custom_target(HDA ALL
                DEPENDS ${PROJECT_SOURCE_DIR}/otls/hGeoPatterns.hda
                SOURCES ${args_files} ${shader_objs} )


# Convert some textures shipped with Houdini for examples
file (GLOB houdini_textures ${HFS}/houdini/pic/texture/brick*.rat)
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/textures)
foreach (houdini_texture ${houdini_textures})
    # cmake_path(GET houdini_texture STEM name)
    get_filename_component ( name ${houdini_texture} NAME_WLE )
    set( texture ${PROJECT_SOURCE_DIR}/textures/${name}.tex)
    add_custom_command(
        COMMAND
            ${HFS}/bin/iconvert ${houdini_texture} ${name}.exr
            &&
            ${RMANTREE}/bin/txmake -format openexr -mode periodic -half ${name}.exr ${texture}
        OUTPUT ${texture}
        MAIN_DEPENDENCY ${houdini_texture}
    )
    list (APPEND textures ${texture})
endforeach ()

add_custom_target(TEXTURES ALL
                DEPENDS ${textures}
                SOURCES ${houdini_textures}
)
