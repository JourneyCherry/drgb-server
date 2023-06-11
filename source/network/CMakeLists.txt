SET ( LIBS
	MyClientSocket
	MyConnectee
	MyConnector
	MyConnectorPool
	MyServerSocket
	MyTCPTLSClient
	MyTCPTLSServer
	MyTCPClient
	MyTCPServer
	MyWebsocketClient
	MyWebsocketServer
	MyWebsocketTLSClient
	MyWebsocketTLSServer
)

FOREACH( LIB IN LISTS LIBS )
	IF ( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${LIB}.cpp" )
		ADD_LIBRARY( ${LIB} EXCLUDE_FROM_ALL ${LIB}.cpp ${LIB}.hpp )
	ELSE()
		ADD_LIBRARY( ${LIB} INTERFACE EXCLUDE_FROM_ALL ${LIB}.hpp )
	ENDIF()
ENDFOREACH()

FIND_PACKAGE ( OpenSSL REQUIRED )

TARGET_LINK_LIBRARIES( MyClientSocket PUBLIC Expected ByteQueue PacketProcessor KeyExchanger Encryptor )
TARGET_LINK_LIBRARIES( MyConnectee PUBLIC ByteQueue PacketProcessor Logger MyTCPServer )
TARGET_LINK_LIBRARIES( MyConnector PUBLIC ByteQueue PacketProcessor Thread Logger MyTCPClient )
TARGET_LINK_LIBRARIES( MyConnectorPool PUBLIC MyConnector ThreadExceptHandler )
TARGET_LINK_LIBRARIES( MyServerSocket PUBLIC MyClientSocket )
TARGET_LINK_LIBRARIES( MyTCPClient PUBLIC MyClientSocket )
TARGET_LINK_LIBRARIES( MyTCPServer PUBLIC MyServerSocket MyTCPClient )
TARGET_LINK_LIBRARIES( MyTCPTLSClient PUBLIC MyClientSocket )
TARGET_LINK_LIBRARIES( MyTCPTLSServer PUBLIC MyServerSocket MyTCPTLSClient )
TARGET_LINK_LIBRARIES( MyWebsocketClient PUBLIC MyClientSocket ErrorCode )
TARGET_LINK_LIBRARIES( MyWebsocketServer PUBLIC MyTCPServer MyWebsocketClient )
TARGET_LINK_LIBRARIES( MyWebsocketTLSClient PUBLIC MyClientSocket ErrorCode )
TARGET_LINK_LIBRARIES( MyWebsocketTLSServer PUBLIC MyTCPServer MyWebsocketTLSClient )

UNSET ( LIBS )