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

set(HOUDINI_PACKAGE_DIR ${RFHTREE}/packages)

include_directories( ${RMANTREE}/include )
link_directories( ${RMANTREE}/lib )
link_libraries(libprman.lib libpxrcore.lib libstats.lib)

include_directories( ${HFS}/toolkit/include )
link_directories( ${HFS}/custom/houdini/dsolib )
link_libraries(libGA.lib libGU.lib libGEO.lib libUT.lib libSYS.lib)

add_definitions(-DSIZEOF_VOID_P=8 -D_USE_MATH_DEFINES -DSESI_LITTLE_ENDIAN)
add_definitions(-DNDEBUG)
add_definitions(-w)


file (GLOB rixplugin_source "src/*.cpp")
file (GLOB args_files "rixplugins/Args/*.args")
file (GLOB shader_headers "osl/include/*.h")
file (GLOB shader_source "osl/*.osl")

foreach (_rixpluginame ${rixplugin_source})
    cmake_path(GET _rixpluginame STEM name)
    add_library( ${name} SHARED  ${_rixpluginame})
    set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE  ${PROJECT_SOURCE_DIR}/rixplugins)
endforeach ()


foreach (oslfile ${shader_source})
    cmake_path(GET oslfile STEM name)
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
                SOURCES ${shader_source} ${shader_headers})


add_custom_command(
    COMMAND  ${CMAKE_COMMAND} -E env HOUDINI_PACKAGE_DIR=${HOUDINI_PACKAGE_DIR}
        ${HFS}/bin/hython ${PROJECT_SOURCE_DIR}/python/build_hda.py
        ${PROJECT_SOURCE_DIR}/otls/hGeoPatterns.hda
        ${PROJECT_SOURCE_DIR}/rixplugins
        ${PROJECT_SOURCE_DIR}/shaders
    OUTPUT ${PROJECT_SOURCE_DIR}/otls/hGeoPatterns.hda
    DEPENDS ${args_files} ${shader_objs} ${PROJECT_SOURCE_DIR}/python/build_hda.py
)

add_custom_target(HDA ALL
                DEPENDS ${PROJECT_SOURCE_DIR}/otls/hGeoPatterns.hda
                SOURCES ${args_files} ${shader_objs} )


file (GLOB houdini_textures ${HFS}/houdini/pic/texture/brick*.rat)
foreach (houdini_texture ${houdini_textures})
    cmake_path(GET houdini_texture STEM name)
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
