cmake_minimum_required(VERSION 3.20)
project(Nida VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Multimedia
    Sql
)

if(WIN32)
    add_definitions(-DNIDA_OS_WINDOWS)
elseif(UNIX)
    find_package(Qt6 REQUIRED COMPONENTS DBus)
    add_definitions(-DNIDA_OS_LINUX)
endif()

set(SOURCES
    src/main.cpp

    src/app/nidaapp.cpp
    src/app/systemtray.cpp

    src/models/prayertimes.cpp

    src/services/apiservice.cpp
    src/services/storageservice.cpp
    src/services/mediaservice.cpp
    src/services/adhanscheduler.cpp

    src/ui/mainwidget.cpp
    src/ui/prayerwidget.cpp
    src/ui/settingsdialog.cpp
    src/ui/adhannotificationwindow.cpp
)

set(HEADERS
    src/app/nidaapp.h
    src/app/systemtray.h

    src/models/prayertimes.h

    src/services/apiservice.h
    src/services/storageservice.h
    src/services/mediaservice.h
    src/services/adhanscheduler.h

    src/ui/mainwidget.h
    src/ui/prayerwidget.h
    src/ui/settingsdialog.h
    src/ui/adhannotificationwindow.h
)

set(UI_FORMS
    src/ui/mainwidget.ui
    src/ui/settingsdialog.ui
)

set(RESOURCES
    resources/resources.qrc
)

if(WIN32)
    list(APPEND SOURCES platform/windows/wmmediacontrol.cpp)
    list(APPEND HEADERS platform/windows/wmmediacontrol.h)
elseif(UNIX)
    list(APPEND SOURCES platform/linux/mpriscontroller.cpp)
    list(APPEND HEADERS platform/linux/mpriscontroller.h)
endif()

qt_add_executable(Nida
    ${SOURCES}
    ${HEADERS}
    ${UI_FORMS}
    ${RESOURCES}
)

target_include_directories(Nida PRIVATE
    src
    src/app
    src/services
    src/models
    src/ui
)

if(WIN32)
    target_include_directories(Nida PRIVATE platform/windows)
elseif(UNIX)
    target_include_directories(Nida PRIVATE platform/linux)
endif()

target_link_libraries(Nida PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
    Qt6::Sql
)

if(WIN32)
    target_link_libraries(Nida PRIVATE user32 winmm)
elseif(UNIX)
    target_link_libraries(Nida PRIVATE Qt6::DBus)
endif()

if(APPLE)
    set_target_properties(Nida PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Nida"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    )
elseif(WIN32)
    set_target_properties(Nida PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# ── Install rules ──────────────────────────────────────────────────────────

include(GNUInstallDirs)

install(TARGETS Nida
    RUNTIME DESTINATION bin
)

install(FILES "${CMAKE_SOURCE_DIR}/packaging/nida.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
)

install(FILES "${CMAKE_SOURCE_DIR}/packaging/nida.appdata.xml"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
)

install(FILES "${CMAKE_SOURCE_DIR}/resources/icons/icon_colored.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps/"
    RENAME "nida.png"
)
install(FILES "${CMAKE_SOURCE_DIR}/resources/icons/icon_colored.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps/"
    RENAME "nida.svg"
)

# Update icon cache on Linux (best-effort, may need root)
if(UNIX AND NOT APPLE)
    install(CODE "execute_process(COMMAND gtk-update-icon-cache -f -t
        \"${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor\"
        ERROR_QUIET OUTPUT_QUIET)")
endif()

# ── CPack ──────────────────────────────────────────────────────────────────

set(CPACK_PACKAGE_NAME "Nida")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Prayer time notification and adhan player")
set(CPACK_PACKAGE_VENDOR "Nida")
set(CPACK_PACKAGE_CONTACT "nida@localhost")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/CMakeLists.txt")
set(CPACK_SET_DESTDIR ON)

# ── DEB ──
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Nida Developers")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libqt6core6 (>= 6.5), libqt6gui6, libqt6widgets6, libqt6network6,"
    "libqt6multimedia6, libqt6sql6, libqt6dbus6, libc6"
)

# ── RPM ──
set(CPACK_RPM_PACKAGE_RELEASE "1")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Utilities")
set(CPACK_RPM_PACKAGE_REQUIRES
    "qt6-qtcore >= 6.5, qt6-qtgui, qt6-qtwidgets, qt6-qtnetwork,"
    "qt6-qtmultimedia, qt6-qtsql, qt6-qtdbus, glibc"
)

# ── Generators ──
set(CPACK_GENERATOR "TGZ;DEB;RPM")

include(CPack)
