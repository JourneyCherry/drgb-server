SET ( LIBS
	MyClientSocket
	MyServerSocket
	MyTCPServer
	MyTCPClient
	MyWebsocketServer
	MyWebsocketClient
	MyConnector
	MyConnectee
)

#Boost::beast는 Headers-only 패키지이기 때문에 별도의 library link가 필요없다.
#FIND_LIBRARY( Boost REQUIRED )

FOREACH( LIB IN LISTS LIBS )
	ADD_LIBRARY ( ${LIB} EXCLUDE_FROM_ALL ${LIB}.cpp ${LIB}.hpp )
ENDFOREACH()

TARGET_LINK_LIBRARIES( MyClientSocket PUBLIC PacketProcessor )
TARGET_LINK_LIBRARIES( MyServerSocket PUBLIC MyClientSocket )
TARGET_LINK_LIBRARIES( MyTCPClient PUBLIC MyClientSocket )
TARGET_LINK_LIBRARIES( MyTCPServer PUBLIC MyServerSocket StackTraceExcept MyTCPClient)
TARGET_LINK_LIBRARIES( MyWebsocketClient PUBLIC MyClientSocket )
TARGET_LINK_LIBRARIES( MyWebsocketServer PUBLIC MyServerSocket StackTraceExcept MyWebsocketClient)