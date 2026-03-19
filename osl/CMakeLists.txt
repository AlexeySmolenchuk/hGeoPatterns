file (GLOB shader_headers "include/*.h")
file (GLOB shader_sources "*.osl")

# compile OSL shaders
foreach (shader ${shader_sources})
    get_filename_component ( shadername ${shader} NAME_WLE )
    # Depfiles supported not for all generators in the earlier versions
    if(CMAKE_VERSION VERSION_LESS "3.20.0")
        add_custom_command(
            COMMAND
                ${RMANTREE}/bin/oslc
                -I${CMAKE_CURRENT_LIST_DIR}/include
                -o ${shadername}.oso
                ${CMAKE_CURRENT_LIST_DIR}/${shadername}.osl
            OUTPUT ${shadername}.oso
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    else()
        # DEPFILE argument is relative to CMAKE_CURRENT_BINARY_DIR
        cmake_policy(SET CMP0116 NEW)
        add_custom_command(
            COMMAND
                ${RMANTREE}/bin/oslc
                -I${CMAKE_CURRENT_LIST_DIR}/include
                -o ${shadername}.oso
                -MMD
                -MF ${shadername}.d
                ${CMAKE_CURRENT_LIST_DIR}/${shadername}.osl
            OUTPUT ${shadername}.oso
            DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/${shadername}.d
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()

    list (APPEND shaders ${CMAKE_CURRENT_BINARY_DIR}/${shadername}.oso)
endforeach ()

install(FILES ${shaders}
        DESTINATION shaders)

install(FILES ${shader_headers}
        DESTINATION osl/include)

add_custom_target(OSL ALL
                DEPENDS ${shaders})

set(shaders_files ${shaders} PARENT_SCOPE)
