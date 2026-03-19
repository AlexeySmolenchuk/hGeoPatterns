include_directories(SYSTEM ${RMANTREE}/include )
link_directories( ${RMANTREE}/lib )

# RenderMan manual recommends not to explicitly link plugins to
# prman libraries and left it unresolved on Linux platform
if (MSVC)
    link_libraries(libprman libpxrcore libstats)
endif()


set (rixplugin_names
    closest
    interpolator
    oceanSampleLayers
    pointCloudFilter
    readAttribute
    samplePoints
    sampleVolume
    )

foreach (rixplugin_name ${rixplugin_names})
    add_library( ${rixplugin_name} SHARED ${rixplugin_name}.cpp)
    target_link_libraries( ${rixplugin_name} Houdini )
    set_target_properties(${rixplugin_name} PROPERTIES
        PREFIX "")
endforeach ()

add_custom_target(RIXPLUGINS DEPENDS ${rixplugin_names})

if (MSVC)
    #Prevent installation .lib files
    install(TARGETS ${rixplugin_names}
            RUNTIME COMPONENT Runtime
            DESTINATION rixplugins)
else()
    install(TARGETS ${rixplugin_names}
            DESTINATION rixplugins)
endif()


