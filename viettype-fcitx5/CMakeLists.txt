cmake_minimum_required(VERSION 3.16)
project(viettype-fcitx5 VERSION 2027.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

# Find Fcitx5
find_package(Fcitx5Core 5.0 REQUIRED)
find_package(Fcitx5Module REQUIRED COMPONENTS Notifications)

# Fcitx5 compiler settings (optional)
include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake"
        OPTIONAL)

# ==================== viettype-engine (Rust static library) ====================
# Build first: cd ../viettype-engine && cargo build --release
# Produces libviettype_engine.a — statically linked into the addon .so
set(VIETTYPE_ENGINE_LIB_DIR "${CMAKE_SOURCE_DIR}/../viettype-engine/target/release"
    CACHE PATH "Path to viettype-engine library directory")

add_library(viettype_engine STATIC IMPORTED)
set_target_properties(viettype_engine PROPERTIES
    IMPORTED_LOCATION "${VIETTYPE_ENGINE_LIB_DIR}/libviettype_engine.a"
)

# ==================== glibc compat shim ====================
# Provides local definitions of arc4random (GLIBC_2.36), __isoc23_strtol
# (GLIBC_2.38), and _dl_find_object (GLIBC_2.35) so the addon runs on
# glibc 2.34+ (Rocky 9 / RHEL 9) even when built on newer distros.
# ==================== Addon shared library ====================
add_library(viettype MODULE src/viettype-fcitx5.cpp src/glibc-compat.c)
target_include_directories(viettype PRIVATE src)
target_link_libraries(viettype
    Fcitx5::Core
    viettype_engine
    pthread dl m    # Rust runtime native deps
)

# Static link C++ and GCC runtimes for maximum portability.
# This eliminates GLIBCXX_3.4.x version requirements, so the .so
# runs on distros with older libstdc++.
target_link_options(viettype PRIVATE -static-libstdc++ -static-libgcc)

# No RPATH needed — engine is statically linked, fcitx5 is the host process
set_target_properties(viettype PROPERTIES
    INSTALL_RPATH ""
    BUILD_RPATH ""
)

# ==================== Install rules ====================

# Addon module (.so) — goes to distro-appropriate fcitx5 addon dir
install(TARGETS viettype
    DESTINATION "${FCITX_INSTALL_ADDONDIR}")

# Addon descriptor → /usr/share/fcitx5/addon/
install(FILES data/viettype-addon.conf
    RENAME viettype.conf
    DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

# Input method entry
install(FILES data/viettype.conf
    DESTINATION "${FCITX_INSTALL_PKGDATADIR}/inputmethod")

# Icon (scalable SVG)
install(FILES data/fcitx-viettype.svg
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps")

# ==================== CPack — .deb & .rpm ====================

set(CPACK_PACKAGE_NAME "viettype-fcitx5")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "VietType Vietnamese Input Method for Fcitx5")
set(CPACK_PACKAGE_DESCRIPTION
    "Vietnamese input method engine (Telex, VNI, VIQR) powered by viettype-engine (Rust). Supports all major Vietnamese charsets.")
set(CPACK_PACKAGE_CONTACT "hophuonglam@outlook.com")
set(CPACK_PACKAGE_VENDOR "VietType")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/phuonglam505/VietType")
set(CPACK_STRIP_FILES ON)

# ---------- DEB ----------
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "VietType <hophuonglam@outlook.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "fcitx5 (>= 5.0)")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS
    "fcitx5-frontend-gtk3, fcitx5-frontend-qt5, fcitx5-config-qt, im-config | im-chooser")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

# ---------- RPM ----------
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_GROUP "System Environment/Libraries")
set(CPACK_RPM_PACKAGE_REQUIRES "fcitx5 >= 5.0")
set(CPACK_RPM_PACKAGE_RECOMMENDS "fcitx5, fcitx5-gtk3, fcitx5-qt5")
set(CPACK_RPM_PACKAGE_SUGGESTS "fcitx5-configtool")
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
# Disable auto-detection of SONAME deps — avoids platform-specific
# libFcitx5Core.so.N version mismatches between build host and target
set(CPACK_RPM_PACKAGE_AUTOREQ OFF)
set(CPACK_RPM_PACKAGE_AUTOPROV OFF)

# Enable both generators
set(CPACK_GENERATOR "DEB;RPM")

install(FILES LICENSE DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/${PROJECT_NAME}")
include(CPack)
