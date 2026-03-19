cmake_minimum_required(VERSION 3.5.0)
project(hGeoPatterns VERSION 1.3 LANGUAGES CXX)

# install directly to the project folder
SET(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/hGeoPatterns${CMAKE_PROJECT_VERSION}")
set(CMAKE_INSTALL_MESSAGE NEVER)

# Please update variables for your OS
if (MSVC)
    # WINDOWS
    SET(HFS "C:/Program Files/Side Effects Software/Houdini 21.0.631" CACHE PATH "Houdini Installation Folder")
    SET(RMANTREE "C:/Program Files/Pixar/RenderManProServer-27.1" CACHE PATH "RendermanProServer Installation Folder")
    SET(RFHTREE "C:/Program Files/Pixar/RenderManForHoudini-27.1" CACHE PATH "RendermanForHoudini Installation Folder")
else()
    # LINUX
    SET(HFS "/opt/hfs20.5.445" CACHE PATH "Houdini Installation Folder")
    SET(RMANTREE "/opt/pixar/RenderManProServer-27.1" CACHE PATH "RendermanProServer Installation Folder")
    SET(RFHTREE "/opt/pixar/RenderManForHoudini-27.1" CACHE PATH "RendermanForHoudini Installation Folder")
    # Not necessary match Houdini version
    SET(RFH_HOUDINI_VERS "20.5.684" CACHE STRING "RendermanForHoudini build")
endif()

# prevent attempt to build right in the project
if ("${PROJECT_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message (FATAL_ERROR "Not allowed to run in-source build!")
endif ()


find_package(Houdini REQUIRED
    PATHS ${HFS}/toolkit/cmake)

# gather sources
file (GLOB args_files "rixplugins/Args/*.args")

add_subdirectory(src)
add_subdirectory(osl)

if (NOT MSVC)
    STRING(CONCAT PYTHONPATH
        "${RFHTREE}/${_houdini_python_version}/${RFH_HOUDINI_VERS}/python${_houdini_python_version}libs:"
        "${RMANTREE}/bin:"
        "${RMANTREE}/lib/python${_houdini_python_version}/site-packages")
endif()

# create HDA
add_custom_command(
    COMMAND  ${CMAKE_COMMAND} -E env
        HOUDINI_PACKAGE_DIR=${RFHTREE}/packages
        PYTHONPATH=${PYTHONPATH}
        ${HFS}/bin/hython
        ${PROJECT_SOURCE_DIR}/python/build_hda.py
        hGeoPatterns.hda
        ${PROJECT_SOURCE_DIR}/rixplugins
        ${CMAKE_BINARY_DIR}/osl
    OUTPUT hGeoPatterns.hda
    DEPENDS ${args_files} ${shaders_files} ${PROJECT_SOURCE_DIR}/python/build_hda.py
)

add_custom_target(HDA ALL
                DEPENDS OSL hGeoPatterns.hda)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/hGeoPatterns.hda DESTINATION otls)

install(DIRECTORY rixplugins DESTINATION .)
install(DIRECTORY vex DESTINATION . )
install(DIRECTORY vop DESTINATION . )
install(FILES hGeoPatterns.json DESTINATION . )
install(FILES LICENSE DESTINATION . )
