cmake_minimum_required(VERSION 3.21)
project(PriceTracker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets Sql Charts)
if(NOT ANDROID)
    # Tests are desktop-only
    find_package(Qt6 REQUIRED COMPONENTS Test)
    enable_testing()
endif()

# Non-GUI logic, shared by the app and the tests
add_library(PriceCore STATIC
    pricefetcher.h pricefetcher.cpp
    pricedatabase.h pricedatabase.cpp
    analysis.h analysis.cpp
    alertmanager.h alertmanager.cpp
    settings.h settings.cpp
)
target_include_directories(PriceCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(PriceCore PUBLIC Qt6::Core Qt6::Network Qt6::Sql)

# The application
# qt_add_executable (instead of add_executable) is required for Android:
# it builds the app as a shared library and provides the "apk" target.
# On desktop it behaves exactly like add_executable.
qt_add_executable(PriceTracker
    main.cpp
    mainwindow.h mainwindow.cpp
    chartwidget.h chartwidget.cpp
    analysiswidget.h analysiswidget.cpp
    alertswidget.h alertswidget.cpp
    settingswidget.h settingswidget.cpp
)
target_link_libraries(PriceTracker PRIVATE PriceCore Qt6::Widgets Qt6::Charts)

set(APP_VERSION "1.0.0")
target_compile_definitions(PriceTracker PRIVATE APP_VERSION="${APP_VERSION}")

include(GNUInstallDirs)

if(ANDROID)
    # Android: no tests, no install() rules, the "apk" target does the packaging
    set_target_properties(PriceTracker PROPERTIES
        QT_ANDROID_VERSION_NAME "${APP_VERSION}"
        QT_ANDROID_VERSION_CODE 1      # increase for every new release
    )

    # HTTPS support libraries. Only used if you cloned
    # https://github.com/KDAB/android_openssl into the project folder
    # (the GitHub workflow step does this). Otherwise it is skipped.
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/android_openssl/android_openssl.cmake")
        include(${CMAKE_CURRENT_SOURCE_DIR}/android_openssl/android_openssl.cmake)
        add_android_openssl_libraries(PriceTracker)
    endif()
else()
    # Unit tests (desktop only)
    add_executable(tst_core tests/tst_core.cpp)
    target_link_libraries(tst_core PRIVATE PriceCore Qt6::Test)
    add_test(NAME tst_core COMMAND tst_core)

    if(WIN32)
        # Windows: no console window, install exe at the root of the install dir
        set_target_properties(PriceTracker PROPERTIES WIN32_EXECUTABLE ON)
        install(TARGETS PriceTracker RUNTIME DESTINATION .)
    else()
        # Linux: standard layout (bin, applications, icons)
        install(TARGETS PriceTracker RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
        install(FILES packaging/linux/PriceTracker.desktop
                DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
        install(FILES packaging/icon.png
                DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
                RENAME PriceTracker.png)
    endif()
endif()