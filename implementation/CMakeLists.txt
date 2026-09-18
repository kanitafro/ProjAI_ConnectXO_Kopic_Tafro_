cmake_minimum_required(VERSION 3.18)

set(SOLUTION_NAME ConnectXO)	#Unijeti naziv solution-a

#BEGIN dio koji ide u sve solutions
project(${SOLUTION_NAME})
set(SOURCE_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(HOME_ROOT $ENV{HOME})
if (WIN32)
	string(REPLACE "\\" "/" HOME_ROOT "${HOME_ROOT}")
endif()
set(NATID_SDK_ROOT ${HOME_ROOT}/natID.SDK)
include(${NATID_SDK_ROOT}/DevEnv/Common.cmake) 
#END dio koji ide u sve solutions

include(${NATID_SDK_ROOT}/DevEnv/natGUI.cmake)

# ==============================================================================
# macOS-specific settings (Windows build unaffected)
# ==============================================================================
if(APPLE)
	# Set minimum macOS version for compatibility
	set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version" FORCE)

	# Enable code signing support (optional, user configurable)
	# Users can enable via: cmake -DENABLE_CODE_SIGNING=ON ..
	option(ENABLE_CODE_SIGNING "Enable macOS code signing" OFF)

	if(ENABLE_CODE_SIGNING)
		message(STATUS "macOS Code Signing: ENABLED")
		message(STATUS "Note: Set team ID via: -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=YOUR_TEAM_ID")
	else()
		message(STATUS "macOS Code Signing: DISABLED (optional for testing)")
	endif()
endif()

include(ConnectXO.cmake)






