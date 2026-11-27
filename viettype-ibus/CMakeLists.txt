cmake_minimum_required(VERSION 3.10)
project(viettype-ibus VERSION 2027.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

# ==================== Find IBus ====================
find_package(PkgConfig REQUIRED)
pkg_check_modules(IBUS REQUIRED ibus-1.0>=1.5)
pkg_check_modules(GLIB REQUIRED glib-2.0)

# ==================== viettype-engine (Rust static library) ====================
# Build first: cd ../viettype-engine && cargo build --release
set(VIETTYPE_ENGINE_LIB_DIR "${CMAKE_SOURCE_DIR}/../viettype-engine/target/release"
    CACHE PATH "Path to viettype-engine library directory")

add_library(viettype_engine STATIC IMPORTED)
set_target_properties(viettype_engine PROPERTIES
    IMPORTED_LOCATION "${VIETTYPE_ENGINE_LIB_DIR}/libviettype_engine.a"
)

# ==================== IBus engine executable ====================
add_executable(ibus-engine-viettype src/viettype-ibus.c src/glibc-compat.c)
target_include_directories(ibus-engine-viettype PRIVATE
    src
    ${IBUS_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
)
target_link_libraries(ibus-engine-viettype
    ${IBUS_LIBRARIES}
    ${GLIB_LIBRARIES}
    viettype_engine
    pthread dl m
)
target_compile_options(ibus-engine-viettype PRIVATE
    ${IBUS_CFLAGS_OTHER}
    ${GLIB_CFLAGS_OTHER}
)

# Static link GCC runtime for portability
target_link_options(ibus-engine-viettype PRIVATE -static-libgcc)

# ==================== Install rules ====================

# Engine install dir — always use absolute path under install prefix
# (pkg-config --variable=libexecdir ibus-1.0 returns empty on Debian 11)
set(IBUS_LIBEXECDIR "${CMAKE_INSTALL_PREFIX}/libexec"
    CACHE PATH "Where to install ibus-engine-viettype")

# IBus component XML dir — /usr/share/ibus/component
set(IBUS_COMPONENT_DIR "${CMAKE_INSTALL_PREFIX}/share/ibus/component"
    CACHE PATH "IBus component XML directory")

# Engine executable
install(TARGETS ibus-engine-viettype
    DESTINATION "${IBUS_LIBEXECDIR}")

# Component XML — configure with actual install path
configure_file(
    "${CMAKE_SOURCE_DIR}/data/viettype.xml.in"
    "${CMAKE_BINARY_DIR}/viettype.xml"
    @ONLY
)
install(FILES "${CMAKE_BINARY_DIR}/viettype.xml"
    DESTINATION "${IBUS_COMPONENT_DIR}")

# Icon (scalable SVG)
install(FILES data/viettype.svg
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps"
    RENAME ibus-viettype.svg)

# ==================== CPack — .deb & .rpm ====================

set(CPACK_PACKAGE_NAME "viettype-ibus")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "VietType Vietnamese Input Method for IBus")
set(CPACK_PACKAGE_DESCRIPTION
    "Vietnamese input method engine (Telex, VNI, VIQR) for IBus framework, powered by viettype-engine (Rust). Supports all major Vietnamese charsets.")
set(CPACK_PACKAGE_CONTACT "hophuonglam@outlook.com")
set(CPACK_PACKAGE_VENDOR "VietType")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/phuonglam505/VietType")
set(CPACK_STRIP_FILES ON)

# ---------- DEB ----------
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "VietType <hophuonglam@outlook.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "ibus (>= 1.5)")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS
    "ibus-gtk, ibus-gtk3, im-config | im-chooser")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

# ---------- RPM ----------
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_GROUP "System Environment/Libraries")
set(CPACK_RPM_PACKAGE_REQUIRES "ibus >= 1.5")
set(CPACK_RPM_PACKAGE_RECOMMENDS "ibus-gtk2, ibus-gtk3")
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_AUTOREQ OFF)
set(CPACK_RPM_PACKAGE_AUTOPROV OFF)

# Enable both generators
set(CPACK_GENERATOR "DEB;RPM")

install(FILES LICENSE DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/${PROJECT_NAME}")
include(CPack)
