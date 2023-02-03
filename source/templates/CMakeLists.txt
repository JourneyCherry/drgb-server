SET ( LIBS
	MyServer
	MyMain
	MyServerOpt
)

FOREACH( LIB IN LISTS LIBS )
	ADD_LIBRARY( ${LIB} ${LIB}.cpp ${LIB}.hpp )
ENDFOREACH()

TARGET_LINK_LIBRARIES ( MyServerOpt PUBLIC MyGetOpt )
TARGET_LINK_LIBRARIES ( MyServer PUBLIC Thread MyClientSocket MyServerSocket MyBytes MyTCPServer MyTCPClient MyWebsocketServer MyWebsocketClient )
TARGET_LINK_LIBRARIES ( MyMain PUBLIC MyLogger MyDaemoner MyConfigParser MyServerOpt )