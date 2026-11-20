cmake_minimum_required(VERSION 3.21)
project(VietType VERSION 0.2.8 LANGUAGES CXX RC)
set(win win32/VietType)
add_executable(VietType WIN32
    ${win}/main.cpp ${win}/AppDelegate.cpp ${win}/MainWindow.cpp ${win}/SystemOptions.cpp
    ${win}/BaseDialog.cpp ${win}/ConvertToolDialog.cpp
    ${win}/MacroDialog.cpp ${win}/VietType.cpp
    ${win}/VietTypeHelper.cpp ${win}/VietTypeManager.cpp ${win}/SystemTrayHelper.cpp
    ${win}/stdafx.cpp ${win}/VietType.rc
    engine/ConvertTool.cpp engine/Engine.cpp engine/Macro.cpp
    engine/SmartSwitchKey.cpp engine/Vietnamese.cpp)
target_include_directories(VietType PRIVATE ${win} "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(VietType PRIVATE cxx_std_17)
target_compile_definitions(VietType PRIVATE UNICODE _UNICODE NOMINMAX WIN32
    "VIETTYPE_VERSION_DISPLAY=L\"${PROJECT_VERSION}\""
    VIETTYPE_VERSION_NUMBER=208)
target_compile_options(VietType PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/utf-8>)
target_link_libraries(VietType PRIVATE comctl32 ole32 uuid shlwapi psapi advapi32 winhttp uxtheme windowsapp)

set_property(TARGET VietType PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
