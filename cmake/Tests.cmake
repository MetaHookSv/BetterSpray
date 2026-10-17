add_executable(BetterSprayGameDataResolverTests tests/gamedata_resolver_tests.cpp src/privatehook.cpp)
target_link_libraries(BetterSprayGameDataResolverTests PRIVATE betterspray_settings)
add_executable(BetterSprayImageLibraryTests tests/image_library_tests.cpp)
target_link_libraries(BetterSprayImageLibraryTests PRIVATE FreeImage LibXml2::LibXml2)
add_custom_command(TARGET BetterSprayImageLibraryTests POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:FreeImage>" "$<TARGET_FILE:LibXml2>" "$<TARGET_FILE_DIR:BetterSprayImageLibraryTests>"
    VERBATIM)
add_executable(BetterSprayPluginLoadSmoke tests/plugin_load_smoke.cpp)
target_link_libraries(BetterSprayPluginLoadSmoke PRIVATE betterspray_settings)
add_dependencies(BetterSprayPluginLoadSmoke BetterSpray)
foreach(test BetterSprayGameDataResolverTests BetterSprayImageLibraryTests BetterSprayPluginLoadSmoke)
    target_compile_features(${test} PRIVATE cxx_std_20)
    target_compile_options(${test} PRIVATE /W4 /WX /UNDEBUG)
    set_target_properties(${test} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/tests")
endforeach()
foreach(scenario success first_missing missing kind_mismatch catalog_conflict null_address error_returns)
    add_test(NAME betterspray_gamedata_${scenario} COMMAND BetterSprayGameDataResolverTests ${scenario})
endforeach()
foreach(scenario PNG WEBP HTML)
    add_test(NAME betterspray_library_${scenario} COMMAND BetterSprayImageLibraryTests ${scenario})
endforeach()
add_test(NAME betterspray_plugin_factory COMMAND BetterSprayPluginLoadSmoke "$<TARGET_FILE:BetterSpray>"
    "$<TARGET_FILE:FreeImage>" "$<TARGET_FILE:LibXml2>" "$<TARGET_FILE:SteamAPIBridge>")

# Exercise the actual Windows file APIs with both process code pages, without
# changing the machine's locale. Locale-name manifests require Windows 11.
foreach(variant UTF8 CP936)
    if(variant STREQUAL "UTF8")
        set(test_locale UTF-8)
        set(test_codepage 65001)
    else()
        set(test_locale zh-CN)
        set(test_codepage 936)
    endif()
    set(test_target BetterSprayUTF8FileSystem${variant}Tests)
    set(test_manifest "${PROJECT_BINARY_DIR}/tests/utf8-filesystem-${variant}.manifest")
    file(GENERATE OUTPUT "${test_manifest}" CONTENT
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\" xmlns:asmv3=\"urn:schemas-microsoft-com:asm.v3\">
  <asmv3:application><asmv3:windowsSettings xmlns=\"http://schemas.microsoft.com/SMI/2019/WindowsSettings\"><activeCodePage>${test_locale}</activeCodePage></asmv3:windowsSettings></asmv3:application>
</assembly>
")
    add_executable(${test_target} tests/utf8_filesystem_tests.cpp)
    target_compile_features(${test_target} PRIVATE cxx_std_20)
    target_include_directories(${test_target} PRIVATE "${METAHOOK_SOURCE_PATH}/include/SourceSDK")
    target_compile_options(${test_target} PRIVATE /W4 /WX /UNDEBUG)
    target_link_options(${test_target} PRIVATE /MANIFEST:EMBED "/MANIFESTINPUT:${test_manifest}")
    set_target_properties(${test_target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/tests")
    add_test(NAME betterspray_utf8_filesystem_${variant} COMMAND ${test_target} ${test_codepage})
    set_tests_properties(betterspray_utf8_filesystem_${variant} PROPERTIES SKIP_RETURN_CODE 77)
endforeach()
get_property(betterspray_tests DIRECTORY PROPERTY TESTS)
set_tests_properties(${betterspray_tests} PROPERTIES TIMEOUT 30)
