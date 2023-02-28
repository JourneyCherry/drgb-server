SET ( LIBS
	MyServer
	MyMain
	MyServerOpt
)

FOREACH( LIB IN LISTS LIBS )
	ADD_LIBRARY( ${LIB} ${LIB}.cpp ${LIB}.hpp )
ENDFOREACH()

TARGET_LINK_LIBRARIES ( MyServerOpt PUBLIC GetArg )
TARGET_LINK_LIBRARIES ( MyServer PUBLIC Thread Logger MyClientSocket MyServerSocket ByteQueue MyTCPServer MyTCPClient MyWebsocketServer MyWebsocketClient )
TARGET_LINK_LIBRARIES ( MyMain PUBLIC Logger Daemonizer ConfigParser MyServerOpt )