cmake_minimum_required(VERSION 3.16)

project(gwi VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(QT_NO_KEYWORDS)
find_package(Qt6 COMPONENTS Quick Charts VirtualKeyboard REQUIRED)

# Detect if building on a RPi
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(IS_RASPBERRY_PI TRUE)
    message(STATUS "Building for Raspberry Pi (${CMAKE_SYSTEM_PROCESSOR})")
else()
    set(IS_RASPBERRY_PI FALSE)
    message(STATUS "Building for development platform (${CMAKE_SYSTEM_PROCESSOR})")
endif()

# OpenCV and LCCV setup
if(IS_RASPBERRY_PI)
    # Set OpenCV_DIR
    set(OpenCV_DIR ${CMAKE_SOURCE_DIR}/opencv-mobile-4.13.0-armlinux/arm-linux-gnueabihf/lib/cmake/opencv4)
    
    find_package(OpenCV REQUIRED)
    message(STATUS "OpenCV found: ${OpenCV_DIR}")
    message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBCAMERA REQUIRED libcamera)
    
    # Find WiringPi
    find_library(WIRINGPI_LIBRARIES NAMES wiringPi)
    if(NOT WIRINGPI_LIBRARIES)
        message(FATAL_ERROR "WiringPi library not found on Raspberry Pi")
    endif()
    add_compile_definitions(HAVE_WIRINGPI)
    add_compile_definitions(HAVE_LCCV)

    include_directories(${LIBCAMERA_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS})
endif()

# -- If Timer is used to displace DMF signals -- #
option(USE_TIMER_MODE "Use timer instead of hardware ISR for sensor readings" ON)

if(USE_TIMER_MODE)
    add_compile_definitions(USE_TIMER)
    message(STATUS "Build mode: TIMER (no DMF signals)")
else()
    message(STATUS "Build mode: ISR (hardware DMF interrupts)")
endif()

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appgwi
    src/main.cpp
    src/ButtonHandler.cpp
)

qt_add_qml_module(appgwi
    URI gwi
    VERSION 1.0
    QML_FILES
        Main.qml
        SOURCES include/ButtonHandler.hpp
        SOURCES include/fkYAML.hpp
        RESOURCES resources/resources.qrc
        SOURCES src/StateManager.cpp
        SOURCES include/StateManager.hpp
        QML_FILES Setup.qml
        QML_FILES AmplificationPlot.qml
        SOURCES include/DataManager.hpp
        SOURCES src/DataManager.cpp
        SOURCES include/SliderHandler.hpp
        SOURCES src/SliderHandler.cpp
        QML_FILES RawData.qml
        SOURCES include/RawDataModel.hpp
        SOURCES src/RawDataModel.cpp
        SOURCES include/HardwareController.hpp
        SOURCES src/HardwareController.cpp
        SOURCES include/RunButtonlEventFilter.hpp
        SOURCES src/RunButtonlEventFilter.cpp
        QML_FILES StandardCurve.qml
        RESOURCES resources/standardCurve.png
        QML_FILES Analysis.qml
        QML_FILES Summary.qml
        SOURCES include/StandardCurveModel.hpp
        SOURCES src/StandardCurveModel.cpp
        SOURCES include/ExperimentModel.hpp
        SOURCES src/ExperimentModel.cpp
        SOURCES include/FluorescenceAnalyzer.hpp
        SOURCES src/FluorescenceAnalyzer.cpp
        RESOURCES resources/templates/empty_experiment.yml
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appgwi PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appgwi
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(appgwi PRIVATE include)

target_link_libraries(appgwi PUBLIC
    Qt6::Quick
    Qt6::Charts
    Qt6::VirtualKeyboard
)

if(IS_RASPBERRY_PI)
    target_link_libraries(appgwi PUBLIC
        -llccv
        ${OpenCV_LIBS}
        ${WIRINGPI_LIBRARIES}
    )
endif()

include(GNUInstallDirs)
install(TARGETS appgwi
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
