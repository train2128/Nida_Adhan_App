cmake_minimum_required(VERSION 3.20)
project(Nida VERSION 1.1.0 LANGUAGES CXX)

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
    Svg
)

if(WIN32)
    add_definitions(-DNIDA_OS_WINDOWS)
elseif(LINUX)
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
    src/services/locationservice.cpp

    src/ui/mainwidget.cpp
    src/ui/prayerwidget.cpp
    src/ui/settingsdialog.cpp
    src/ui/adhannotificationwindow.cpp
    src/ui/theme.cpp
)

set(HEADERS
    src/app/nidaapp.h
    src/app/systemtray.h

    src/models/prayertimes.h

    src/services/apiservice.h
    src/services/storageservice.h
    src/services/mediaservice.h
    src/services/adhanscheduler.h
    src/services/locationservice.h

    src/ui/mainwidget.h
    src/ui/prayerwidget.h
    src/ui/settingsdialog.h
    src/ui/adhannotificationwindow.h
    src/ui/theme.h
)

set(UI_FORMS
    src/ui/mainwidget.ui
    src/ui/settingsdialog.ui
)

set(RESOURCES
    resources/resources.qrc
)

if(WIN32)
    option(NIDA_ENABLE_WIN_ICON "Embed Windows .exe icon" ON)
    list(APPEND SOURCES platform/windows/wmmediacontrol.cpp)
    list(APPEND HEADERS platform/windows/wmmediacontrol.h)
    # NOTE: do NOT list resources/win/nida.rc in SOURCES.
    # CMake's auto windres rule passes all target -I dirs, and MinGW
    # windres/gcc splits '-I "D:/Personal Files/..."' on the space,
    # breaking builds in paths with spaces (QtCreator default).
    # We compile the icon manually below with no -I/-D flags.
    set(NIDA_WIN_ICON_RC "${CMAKE_CURRENT_SOURCE_DIR}/resources/win/nida.rc")
    set(NIDA_WIN_ICON_OBJ "${CMAKE_CURRENT_BINARY_DIR}/nida_icon.obj")
elseif(LINUX)
    list(APPEND SOURCES platform/linux/mpriscontroller.cpp)
    list(APPEND HEADERS platform/linux/mpriscontroller.h)
endif()

qt_add_executable(Nida
    ${SOURCES}
    ${HEADERS}
    ${UI_FORMS}
    ${RESOURCES}
)

# Space-safe Windows icon: manual windres with no -I/-D (verified to work
# in "D:/Personal Files/..." with both MinGW Makefiles and Ninja).
# nida.rc is just: 1 ICON "icon.ico" (no preprocessor needs).
if(WIN32 AND NIDA_ENABLE_WIN_ICON)
    if(CMAKE_RC_COMPILER)
        add_custom_command(
            OUTPUT "${NIDA_WIN_ICON_OBJ}"
            COMMAND "${CMAKE_RC_COMPILER}" -O coff
                    -i "${NIDA_WIN_ICON_RC}"
                    -o "${NIDA_WIN_ICON_OBJ}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources/win"
            DEPENDS "${NIDA_WIN_ICON_RC}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/resources/win/icon.ico"
            COMMENT "Embedding Windows icon (space-safe windres)"
            VERBATIM
        )
        target_sources(Nida PRIVATE "${NIDA_WIN_ICON_OBJ}")
    else()
        message(WARNING "NIDA_ENABLE_WIN_ICON=ON but no RC compiler found; building without embedded icon")
    endif()
endif()

target_include_directories(Nida PRIVATE
    src
    src/app
    src/services
    src/models
    src/ui
)

if(WIN32)
    target_include_directories(Nida PRIVATE platform/windows)
elseif(LINUX)
    target_include_directories(Nida PRIVATE platform/linux)
endif()

target_link_libraries(Nida PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
    Qt6::Sql
    Qt6::Svg
)

if(WIN32)
    target_link_libraries(Nida PRIVATE user32 winmm ole32)
elseif(LINUX)
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

# Copy the Qt DLLs/plugins next to Nida.exe after each build, so the build
# folder runs on its own (e.g. "Nida.exe --test-adhan" from a terminal), not
# only when launched from Qt Creator. windeployqt skips files already up to date.
if(WIN32)
    option(NIDA_WINDEPLOY_ON_BUILD "Run windeployqt after building" ON)
    if(NIDA_WINDEPLOY_ON_BUILD)
        if(TARGET Qt6::windeployqt)
            set(NIDA_WINDEPLOYQT "$<TARGET_FILE:Qt6::windeployqt>")
        else()
            find_program(NIDA_WINDEPLOYQT windeployqt HINTS "${QT6_INSTALL_PREFIX}/bin")
        endif()
        if(NIDA_WINDEPLOYQT)
            add_custom_command(TARGET Nida POST_BUILD
                COMMAND "${NIDA_WINDEPLOYQT}" --no-translations --verbose 0
                        "$<TARGET_FILE:Nida>"
                COMMENT "Deploying Qt runtime next to Nida.exe"
                VERBATIM
            )
        else()
            message(WARNING "windeployqt not found; the build folder needs Qt on PATH to run")
        endif()
    endif()
endif()

# ── Install rules ──────────────────────────────────────────────────────────

include(GNUInstallDirs)

install(TARGETS Nida
    RUNTIME DESTINATION bin
)

# Linux-only desktop integration (keep Windows installs clean)
if(UNIX AND NOT APPLE)
    install(FILES "${CMAKE_SOURCE_DIR}/packaging/nida.desktop"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
    )

    install(FILES "${CMAKE_SOURCE_DIR}/packaging/nida.appdata.xml"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
        RENAME "io.github.train2128.Nida.metainfo.xml"
    )

    install(FILES "${CMAKE_SOURCE_DIR}/resources/icons/icon_colored.png"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps/"
        RENAME "nida.png"
    )
endif()
# NOTE: no SVG source exists, so do not install a fake nida.svg.
# For AppImage builds use: -DCMAKE_INSTALL_PREFIX=/usr and DESTDIR=AppDir.

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
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
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
if(WIN32)
    set(CPACK_GENERATOR "NSIS")
else()
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
endif()

include(CPack)
