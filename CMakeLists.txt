cmake_minimum_required(VERSION 3.21)
project(VietType VERSION 0.2.18 LANGUAGES CXX RC)
set(win win32/VietType)
add_executable(VietType WIN32
    ${win}/main.cpp ${win}/AppDelegate.cpp ${win}/MainWindow.cpp ${win}/SystemOptions.cpp ${win}/SelfUpdate.cpp
    ${win}/BaseDialog.cpp ${win}/ConvertToolDialog.cpp
    ${win}/MacroDialog.cpp ${win}/VietType.cpp
    ${win}/VietTypeHelper.cpp ${win}/VietTypeManager.cpp ${win}/SystemTrayHelper.cpp
    ${win}/stdafx.cpp ${win}/VietType.rc
    third_party/vnconv/byteio.cpp third_party/vnconv/charset.cpp third_party/vnconv/convert.cpp
    third_party/vnconv/data.cpp third_party/vnconv/pattern.cpp engine/ClipboardEncoding.cpp
    engine/ConvertTool.cpp engine/Engine.cpp engine/Macro.cpp
    engine/SmartSwitchKey.cpp engine/Vietnamese.cpp)
target_include_directories(VietType PRIVATE ${win} third_party/vnconv "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(VietType PRIVATE cxx_std_17)
math(EXPR viettype_build_number "${PROJECT_VERSION_MAJOR} * 10000 + ${PROJECT_VERSION_MINOR} * 100 + ${PROJECT_VERSION_PATCH}")
target_compile_definitions(VietType PRIVATE UNICODE _UNICODE NOMINMAX WIN32 VIETTYPE_STATIC_UNIKEY _CRT_SECURE_NO_WARNINGS
    "VIETTYPE_VERSION_DISPLAY=L\"${PROJECT_VERSION}\""
    VIETTYPE_VERSION_NUMBER=${viettype_build_number})
target_compile_options(VietType PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/utf-8>)
target_link_libraries(VietType PRIVATE comctl32 ole32 uuid shlwapi psapi advapi32 winhttp uxtheme windowsapp gdi32 bcrypt version)

set_property(TARGET VietType PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
