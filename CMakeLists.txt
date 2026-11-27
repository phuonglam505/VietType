cmake_minimum_required(VERSION 3.21)
project(VietType LANGUAGES CXX)
include(GNUInstallDirs)

if(NOT WIN32)
    message(FATAL_ERROR "VietType requires Windows.")
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)
find_program(CARGO_EXECUTABLE cargo REQUIRED)
qt_standard_project_setup()

qt_add_executable(VietTypeSettings WIN32 settings/main.cpp)
qt_add_resources(VietTypeSettings "viettype_icon"
    PREFIX "/"
    BASE "viettype-windows"
    FILES "viettype-windows/v.ico"
)
qt_add_resources(VietTypeSettings "settings_indicators"
    PREFIX "/"
    BASE "settings"
    FILES "settings/checkbox-checked.png"
)
target_compile_features(VietTypeSettings PRIVATE cxx_std_17)
target_link_libraries(VietTypeSettings PRIVATE Qt6::Widgets)
if(MSVC)
    target_compile_options(VietTypeSettings PRIVATE /utf-8)
endif()

set(engine_target_dir "${CMAKE_CURRENT_BINARY_DIR}/cargo")
set(engine_profile "$<IF:$<CONFIG:Debug>,dev,release>")
set(engine_output "$<IF:$<CONFIG:Debug>,debug,release>")
add_custom_target(VietTypeEngine ALL
    COMMAND "${CARGO_EXECUTABLE}" build --locked --profile "${engine_profile}"
            --manifest-path "${CMAKE_CURRENT_SOURCE_DIR}/viettype-windows/Cargo.toml"
            --target-dir "${engine_target_dir}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${engine_target_dir}/${engine_output}/VietType.exe"
            "$<TARGET_FILE_DIR:VietTypeSettings>/VietType.exe"
    VERBATIM
)
add_dependencies(VietTypeEngine VietTypeSettings)

install(TARGETS VietTypeSettings RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(PROGRAMS "${engine_target_dir}/${engine_output}/VietType.exe" DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES LICENSE DESTINATION .)
qt_generate_deploy_app_script(TARGET VietTypeSettings OUTPUT_SCRIPT deploy_settings)
install(SCRIPT "${deploy_settings}")
