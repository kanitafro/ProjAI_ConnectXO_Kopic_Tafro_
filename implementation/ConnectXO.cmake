set(CONNECTXO_PO_NAME ConnectXO)				#Naziv prvog projekta u solution-u

file(GLOB CONNECTXO_PO_SOURCES  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)
file(GLOB CONNECTXO_PO_INCS  ${CMAKE_CURRENT_LIST_DIR}/src/*.h)
file(GLOB CONNECTXO_PO_INC_TD  ${NATID_SDK_INC}/td/*.h)
file(GLOB CONNECTXO_PO_INC_GUI ${NATID_SDK_INC}/gui/*.h)

file(GLOB CONNECTXO_PO_INC_THREAD  ${NATID_SDK_INC}/thread/*.h)
file(GLOB CONNECTXO_PO_INC_CNT  ${NATID_SDK_INC}/cnt/*.h)
file(GLOB CONNECTXO_PO_INC_FO  ${NATID_SDK_INC}/fo/*.h)
file(GLOB CONNECTXO_PO_INC_XML  ${NATID_SDK_INC}/xml/*.h)

#Application icon
set(CONNECTXO_PO_PLIST  ${CMAKE_CURRENT_LIST_DIR}/res/appIcon/AppIcon.plist)
if(WIN32)
	set(CONNECTXO_PO_WINAPP_ICON ${CMAKE_CURRENT_LIST_DIR}/res/appIcon/winAppIcon.rc)
else()
	set(CONNECTXO_PO_WINAPP_ICON ${CMAKE_CURRENT_LIST_DIR}/res/appIcon/winAppIcon.cpp)
endif()

# add executable
add_executable(${CONNECTXO_PO_NAME} ${CONNECTXO_PO_INCS} ${CONNECTXO_PO_SOURCES} ${CONNECTXO_PO_INC_TD} ${CONNECTXO_PO_INC_THREAD} 
				${CONNECTXO_PO_INC_CNT} ${CONNECTXO_PO_INC_FO} ${CONNECTXO_PO_INC_GUI} ${CONNECTXO_PO_INC_XML} ${CONNECTXO_PO_WINAPP_ICON})

target_include_directories(${CONNECTXO_PO_NAME} PRIVATE ${NATID_SDK_INC})

source_group("inc"            FILES ${CONNECTXO_PO_INCS})
source_group("inc\\td"        FILES ${CONNECTXO_PO_INC_TD})
source_group("inc\\cnt"        FILES ${CONNECTXO_PO_INC_CNT})
source_group("inc\\fo"        FILES ${CONNECTXO_PO_INC_FO})
source_group("inc\\gui"        FILES ${CONNECTXO_PO_INC_GUI})
source_group("inc\\thread"        FILES ${CONNECTXO_PO_INC_THREAD})
source_group("inc\\xml"        FILES ${CONNECTXO_PO_INC_XML})
source_group("src"            FILES ${CONNECTXO_PO_SOURCES})

target_link_libraries(${CONNECTXO_PO_NAME} 
    debug ${MU_LIB_DEBUG} ${NATGUI_LIB_DEBUG} 
    optimized ${MU_LIB_RELEASE} ${NATGUI_LIB_RELEASE}
)

setTargetPropertiesForGUIApp(${CONNECTXO_PO_NAME} ${CONNECTXO_PO_PLIST})

setAppIcon(${CONNECTXO_PO_NAME} ${CMAKE_CURRENT_LIST_DIR})

setIDEPropertiesForGUIExecutable(${CONNECTXO_PO_NAME} ${CMAKE_CURRENT_LIST_DIR})

setPlatformDLLPath(${CONNECTXO_PO_NAME})