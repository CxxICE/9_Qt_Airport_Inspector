cmake_minimum_required(VERSION 3.5)

project(Qt_Airport_Inspector VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent Charts Sql)


set(PROJECT_SOURCES
        main.cpp
		airplane.o
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
		statistic.h
		statistic.cpp
		statistic.ui
		airportdb.h
		airportdb.cpp
		structs.h
		img.qrc		
		logindialogsql.cpp
		logindialogsql.h
		logindialogsql.ui
)

qt_add_executable(Qt_Airport_Inspector
	MANUAL_FINALIZATION
	${PROJECT_SOURCES})

target_link_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/PG_Libs)
target_link_libraries(Qt_Airport_Inspector PRIVATE	Qt6::Widgets
	                                                Qt6::Sql
													Qt6::Concurrent
													Qt6::Charts)

set_target_properties(Qt_Airport_Inspector PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)


qt_finalize_executable(Qt_Airport_Inspector)

