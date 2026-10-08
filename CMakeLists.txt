cmake_minimum_required(VERSION 3.16)
project(PriceTracker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets Sql Charts Test)
enable_testing()

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
add_executable(PriceTracker
    main.cpp
    mainwindow.h mainwindow.cpp
    chartwidget.h chartwidget.cpp
    analysiswidget.h analysiswidget.cpp
    alertswidget.h alertswidget.cpp
    settingswidget.h settingswidget.cpp
)
target_link_libraries(PriceTracker PRIVATE PriceCore Qt6::Widgets Qt6::Charts)

# Unit tests
add_executable(tst_core tests/tst_core.cpp)
target_link_libraries(tst_core PRIVATE PriceCore Qt6::Test)
add_test(NAME tst_core COMMAND tst_core)


set(APP_VERSION "1.0.0")
target_compile_definitions(PriceTracker PRIVATE APP_VERSION="${APP_VERSION}")

include(GNUInstallDirs)

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