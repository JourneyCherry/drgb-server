SET ( LIBS
	MyMain
	MyServer
	MyServerOpt
)

FOREACH( LIB IN LISTS LIBS )
	IF ( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${LIB}.cpp" )
		ADD_LIBRARY( ${LIB} EXCLUDE_FROM_ALL ${LIB}.cpp ${LIB}.hpp )
	ELSE()
		ADD_LIBRARY( ${LIB} INTERFACE EXCLUDE_FROM_ALL ${LIB}.hpp )
	ENDIF()
ENDFOREACH()

TARGET_LINK_LIBRARIES ( MyMain PUBLIC Daemonizer ConfigParser MyServerOpt Thread )
TARGET_LINK_LIBRARIES ( MyServer PUBLIC FixedPool Thread Logger MyTCPServer MyWebsocketServer MyConnectorPool )
TARGET_LINK_LIBRARIES ( MyServerOpt PUBLIC GetArg Logger )

UNSET ( LIBS )