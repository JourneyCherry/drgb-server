SET ( LIBS
	MyClientSocket
	MyConnectee
	MyConnector
	MyServerSocket
	MyTCPClient
	MyTCPServer
	MyWebsocketClient
	MyWebsocketServer
)

FOREACH( LIB IN LISTS LIBS )
	IF ( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${LIB}.cpp" )
		ADD_LIBRARY( ${LIB} EXCLUDE_FROM_ALL ${LIB}.cpp ${LIB}.hpp )
	ELSE()
		ADD_LIBRARY( ${LIB} INTERFACE EXCLUDE_FROM_ALL ${LIB}.hpp )
	ENDIF()
ENDFOREACH()

TARGET_LINK_LIBRARIES( MyClientSocket PUBLIC Expected ByteQueue PacketProcessor )
TARGET_LINK_LIBRARIES( MyConnectee PUBLIC Invoker ByteQueue PacketProcessor VariadicPool Logger MyTCPServer )
TARGET_LINK_LIBRARIES( MyConnector PUBLIC Invoker ByteQueue PacketProcessor Thread Logger MyTCPClient )
TARGET_LINK_LIBRARIES( MyServerSocket PUBLIC MyClientSocket )
TARGET_LINK_LIBRARIES( MyTCPClient PUBLIC MyClientSocket )
TARGET_LINK_LIBRARIES( MyTCPServer PUBLIC MyServerSocket MyTCPClient )
TARGET_LINK_LIBRARIES( MyWebsocketClient PUBLIC MyClientSocket ErrorCode )
TARGET_LINK_LIBRARIES( MyWebsocketServer PUBLIC MyServerSocket MyWebsocketClient)