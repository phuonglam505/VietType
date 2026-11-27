cmake_minimum_required(VERSION 3.20)
project(viettype-macos LANGUAGES C OBJC)

set(CMAKE_C_STANDARD 11)
set(CMAKE_OBJC_STANDARD 11)

# ==================== viettype-engine (Rust static lib) ====================
set(ENGINE_DIR "${CMAKE_SOURCE_DIR}/../viettype-engine")
set(VIETTYPE_ENGINE_ARCHIVE "" CACHE FILEPATH "Prebuilt Rust engine archive")
if(VIETTYPE_ENGINE_ARCHIVE)
    set(ENGINE_LIB "${VIETTYPE_ENGINE_ARCHIVE}")
else()
    set(ENGINE_LIB "${ENGINE_DIR}/target/release/libviettype_engine.a")
    add_custom_target(viettype_engine_build
        COMMAND cargo build --release --locked
        WORKING_DIRECTORY "${ENGINE_DIR}"
        BYPRODUCTS "${ENGINE_LIB}"
        COMMENT "Building viettype-engine (Rust)"
    )
endif()

add_library(viettype_engine STATIC IMPORTED)
set_target_properties(viettype_engine PROPERTIES IMPORTED_LOCATION "${ENGINE_LIB}")
if(TARGET viettype_engine_build)
    add_dependencies(viettype_engine viettype_engine_build)
endif()

# ==================== VietType.app (CGEventTap) ====================
set(APP_NAME "VietType")
set(BUNDLE_ID "com.github.phuonglam505.viettype")

add_executable(${APP_NAME} MACOSX_BUNDLE
    src/main.m
    src/VietTypeEventTap.m
    src/VietTypeAppDelegate.m
    src/VietTypePreferences.m
)

target_include_directories(${APP_NAME} PRIVATE
    src
    "${ENGINE_DIR}/target/release"
)

# Frameworks — CGEventTap cần ApplicationServices, không cần InputMethodKit
find_library(COCOA_FW Cocoa REQUIRED)
find_library(CARBON_FW Carbon REQUIRED)
find_library(APPSERVICES_FW ApplicationServices REQUIRED)

target_link_libraries(${APP_NAME} PRIVATE
    viettype_engine
    ${COCOA_FW}
    ${CARBON_FW}
    ${APPSERVICES_FW}
    "-framework Security"
    "-framework SystemConfiguration"
    resolv
)

# Info.plist
set_target_properties(${APP_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/resources/Info.plist"
    MACOSX_BUNDLE_BUNDLE_NAME "${APP_NAME}"
    MACOSX_BUNDLE_GUI_IDENTIFIER "${BUNDLE_ID}"
    MACOSX_BUNDLE_BUNDLE_VERSION "2027.1.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "2027.1.0"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
)

# Copy icon vào bundle
set(ICON_FILE "${CMAKE_SOURCE_DIR}/resources/VietType.icns")
if(EXISTS "${ICON_FILE}")
    target_sources(${APP_NAME} PRIVATE "${ICON_FILE}")
    set_source_files_properties("${ICON_FILE}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
endif()

# Copy .tiff menu bar icon vào bundle
set(TIFF_FILE "${CMAKE_SOURCE_DIR}/resources/VietType.tiff")
if(EXISTS "${TIFF_FILE}")
    target_sources(${APP_NAME} PRIVATE "${TIFF_FILE}")
    set_source_files_properties("${TIFF_FILE}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
endif()

# Fallback: copy PNG vào bundle nếu .icns/.tiff chưa có
set(PNG_FILE "${CMAKE_SOURCE_DIR}/resources/VietType.png")
if(EXISTS "${PNG_FILE}")
    target_sources(${APP_NAME} PRIVATE "${PNG_FILE}")
    set_source_files_properties("${PNG_FILE}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
endif()

# Install: copy vào /Applications/ (CGEventTap app, không phải Input Method)
install(TARGETS ${APP_NAME}
    BUNDLE DESTINATION "/Applications"
)

# Ad-hoc codesign sau build
add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:${APP_NAME}>"
    COMMENT "Ad-hoc codesign VietType.app"
)
