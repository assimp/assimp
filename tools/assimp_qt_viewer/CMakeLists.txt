set(PROJECT_VERSION "")
project(assimp_qt_viewer)

cmake_minimum_required(VERSION 2.6)

FIND_PACKAGE(OpenGL QUIET)

# Qt5 version
FIND_PACKAGE(Qt5 COMPONENTS Gui Widgets OpenGL QUIET)

SET(VIEWER_BUILD:BOOL FALSE)

IF(  Qt5Widgets_FOUND AND OPENGL_FOUND)
	SET(VIEWER_BUILD TRUE)
ELSE( Qt5Widgets_FOUND AND OPENGL_FOUND)
	SET ( ASSIMP_QT_VIEWER_DEPENDENCIES "")

	IF (NOT Qt5_FOUND)
		SET ( ASSIMP_QT_VIEWER_DEPENDENCIES "${ASSIMP_QT_VIEWER_DEPENDENCIES} Qt5")
	ENDIF (NOT Qt5_FOUND)

	IF (NOT OPENGL_FOUND)
		SET ( ASSIMP_QT_VIEWER_DEPENDENCIES "${ASSIMP_QT_VIEWER_DEPENDENCIES} OpengGL")
	ENDIF (NOT OPENGL_FOUND)

	MESSAGE (WARNING "Build of assimp_qt_viewer is disabled. Unsatisfied dendencies: ${ASSIMP_QT_VIEWER_DEPENDENCIES}")
ENDIF( Qt5Widgets_FOUND AND OPENGL_FOUND)

IF(VIEWER_BUILD)
	INCLUDE_DIRECTORIES(
		${Assimp_SOURCE_DIR}/include
		${Assimp_SOURCE_DIR}/code
		${CMAKE_CURRENT_BINARY_DIR}
		${CMAKE_SOURCE_DIR}
		${OPENGL_INCLUDE_DIR}
		${IL_INCLUDE_DIR}
	)

	LINK_DIRECTORIES(${Assimp_BINARY_DIR})
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pedantic -Wall")

	SET(assimp_qt_viewer_SRCS 
        main.cpp 
        loggerview.hpp
        loggerview.cpp 
        glview.hpp
        glview.cpp 
        mainwindow.hpp
        mainwindow.cpp
    )

	MESSAGE("assimp_qt_viewer use Qt5")
	INCLUDE_DIRECTORIES(${Qt5Widgets_INCLUDES})
	qt5_wrap_ui(UISrcs mainwindow.ui)
	qt5_wrap_cpp(MOCrcs mainwindow.hpp glview.hpp)

	add_executable(${PROJECT_NAME} ${assimp_qt_viewer_SRCS} ${UISrcs} ${MOCrcs})
	target_link_libraries(${PROJECT_NAME} Qt5::Gui Qt5::Widgets Qt5::OpenGL ${IL_LIBRARIES} ${OPENGL_LIBRARIES} assimp)

	IF(WIN32) # Check if we are on Windows
		IF(MSVC) # Check if we are using the Visual Studio compiler
			#set_target_properties(TestProject PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
		ELSEIF(CMAKE_COMPILER_IS_GNUCXX)
			# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows") # Not tested
		ELSE()
			MESSAGE(SEND_ERROR "You are using an unsupported Windows compiler! (Not MSVC or GCC)")
		ENDIF()
	ELSEIF(UNIX)
		# Nothing special required
	ELSE()
		MESSAGE(SEND_ERROR "You are on an unsupported platform! (Not Win32 or Unix)")
	ENDIF()

	SET_PROPERTY(TARGET ${PROJECT_NAME} PROPERTY DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
	INSTALL(TARGETS assimp_qt_viewer DESTINATION "${ASSIMP_BIN_INSTALL_DIR}")
ENDIF(VIEWER_BUILD)
