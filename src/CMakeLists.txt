#set the minimum cmake version required
CMAKE_MINIMUM_REQUIRED (VERSION 2.8.11)

#set the name of the project
PROJECT (fsm)

#####################################################################
#enable warning for MSVC and gcc

if (MSVC)
	# Force to always compile with W4
	if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string (REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else ()
		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall")
	endif ()
elseif (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

#####################################################################
#enable c++ 11 for MSVC and gcc

include (CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG ("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG ("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if (COMPILER_SUPPORTS_CXX11)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif (COMPILER_SUPPORTS_CXX0X)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else ()
	message (STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif ()

#####################################################################

find_package (Qt5Widgets REQUIRED)

#set the root source diectory as include directory
include_directories (${CMAKE_SOURCE_DIR})
include_directories (${CMAKE_BINARY_DIR})

#add every subdirectory to cmake
add_subdirectory (fsm)
add_subdirectory (interface)
add_subdirectory (main)
add_subdirectory (sets)
add_subdirectory (trees)
add_subdirectory (window)

#copy every fsm file to the build directory
configure_file (../resources/fsm.fsm									main/fsm.fsm									COPYONLY)
configure_file (../resources/fsm.in										main/fsm.in										COPYONLY)
configure_file (../resources/fsm.out									main/fsm.out									COPYONLY)
configure_file (../resources/fsm.state									main/fsm.state									COPYONLY)
configure_file (../resources/fsma.fsm									main/fsma.fsm									COPYONLY)
configure_file (../resources/fsma.in									main/fsma.in									COPYONLY)
configure_file (../resources/fsma.out									main/fsma.out									COPYONLY)
configure_file (../resources/fsma.state									main/fsma.state									COPYONLY)
configure_file (../resources/fsmb.fsm									main/fsmb.fsm									COPYONLY)
configure_file (../resources/fsmb.in									main/fsmb.in									COPYONLY)
configure_file (../resources/fsmb.out									main/fsmb.out									COPYONLY)
configure_file (../resources/fsmb.state									main/fsmb.state									COPYONLY)
configure_file (../resources/fsmGillA7.fsm								main/fsmGillA7.fsm								COPYONLY)
configure_file (../resources/fsmGillA7.in								main/fsmGillA7.in								COPYONLY)
configure_file (../resources/fsmGillA7.out								main/fsmGillA7.out								COPYONLY)
configure_file (../resources/fsmGillA7.state							main/fsmGillA7.state							COPYONLY)

if (MSVC)
	configure_file (../resources/dll/Qt5Core.dll						main/Qt5Core.dll								COPYONLY)
	configure_file (../resources/dll/Qt5Cored.dll						main/Qt5Cored.dll								COPYONLY)
	configure_file (../resources/dll/Qt5Gui.dll							main/Qt5Gui.dll									COPYONLY)
	configure_file (../resources/dll/Qt5Guid.dll						main/Qt5Guid.dll								COPYONLY)
	configure_file (../resources/dll/Qt5Widgets.dll						main/Qt5Widgets.dll								COPYONLY)
	configure_file (../resources/dll/Qt5Widgetsd.dll					main/Qt5Widgetsd.dll							COPYONLY)
	configure_file (../resources/dll/platforms/qwindows.dll				main/platforms/qwindows.dll						COPYONLY)
	configure_file (../resources/dll/platforms/qwindowsd.dll			main/platforms/qwindowsd.dll					COPYONLY)
endif()

