cmake_minimum_required(VERSION 3.0.0)
project(hGeoPatterns VERSION 1.1)

# Please update variables for your OS
if (MSVC)
    # WINDOWS
    SET(HFS "C:/Program Files/Side Effects Software/Houdini 19.5.605" CACHE PATH "Houdini Installation Folder")
    SET(RMANTREE "C:/Program Files/Pixar/RenderManProServer-25.2" CACHE PATH "RendermanProServer Installation Folder")
    SET(RFHTREE "C:/Program Files/Pixar/RenderManForHoudini-25.2" CACHE PATH "RendermanForHoudini Installation Folder")
else()
    # LINUX
    SET(HFS "/opt/hfs19.0.589" CACHE PATH "Houdini Installation Folder")
    SET(RMANTREE "/opt/pixar/RenderManProServer-24.4" CACHE PATH "RendermanProServer Installation Folder")
    SET(RFHTREE "/opt/pixar/RenderManForHoudini-24.4-py3" CACHE PATH "RendermanForHoudini Installation Folder")
endif()

# prevent attempt to build right in the project
if ("${PROJECT_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message (FATAL_ERROR "Not allowed to run in-source build!")
endif ()


find_package(Houdini REQUIRED
    PATHS ${HFS}/toolkit/cmake)

# Houdini and RFH can setup PYTHONPATH for us (Windows only)
set(HOUDINI_PACKAGE_DIR ${RFHTREE}/packages)

include_directories(SYSTEM ${RMANTREE}/include )
link_directories( ${RMANTREE}/lib )
add_definitions(-DNDEBUG)


if (MSVC)
    link_libraries(libprman libpxrcore libstats)
else()
    link_libraries(prman pxrcore stats)
endif()

# gather sources
file (GLOB args_files "rixplugins/Args/*.args")
file (GLOB shader_headers "osl/include/*.h")
file (GLOB shader_sources "osl/*.osl")

set (rixplugin_names
    samplePoints
    readAttribute
    closest
    interpolator)

foreach (rixplugin_name ${rixplugin_names})
    add_library( ${rixplugin_name} SHARED src/${rixplugin_name}.cpp)
    target_link_libraries( ${rixplugin_name} Houdini )
    set_target_properties(${rixplugin_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/rixplugins
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/rixplugins
        PREFIX "")
endforeach ()

add_custom_target(RIXPLUGINS DEPENDS ${rixplugin_names})


# compile OSL shaders
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/shaders)
foreach (oslfile ${shader_sources})
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
if (MSVC)
    add_custom_command(
        COMMAND  ${CMAKE_COMMAND} -E env HOUDINI_PACKAGE_DIR=${HOUDINI_PACKAGE_DIR}
            ${HFS}/bin/hython
            ${PROJECT_SOURCE_DIR}/python/build_hda.py
            ${PROJECT_SOURCE_DIR}/otls/hGeoPatterns.hda
            ${PROJECT_SOURCE_DIR}/rixplugins
            ${PROJECT_SOURCE_DIR}/shaders
        OUTPUT ${PROJECT_SOURCE_DIR}/otls/hGeoPatterns.hda
        DEPENDS ${args_files} ${shader_objs} ${PROJECT_SOURCE_DIR}/python/build_hda.py
    )
else()
    add_custom_command(
        COMMAND  ${CMAKE_COMMAND} -E env PATH="${HFS}/bin:$ENV{PATH}"
            hython
            ${PROJECT_SOURCE_DIR}/python/build_hda.py
            ${PROJECT_SOURCE_DIR}/otls/hGeoPatterns.hda
            ${PROJECT_SOURCE_DIR}/rixplugins
            ${PROJECT_SOURCE_DIR}/shaders
        OUTPUT ${PROJECT_SOURCE_DIR}/otls/hGeoPatterns.hda
        DEPENDS ${args_files} ${shader_objs} ${PROJECT_SOURCE_DIR}/python/build_hda.py
    )
endif()

add_custom_target(HDA ALL
                DEPENDS ${PROJECT_SOURCE_DIR}/otls/hGeoPatterns.hda
                SOURCES ${args_files} ${shader_objs} )

