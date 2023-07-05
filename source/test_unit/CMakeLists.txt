FIND_PACKAGE ( GTest REQUIRED )

SET ( TESTS
	ByteQueue_Test
	ConfigParser_Test
	Daemonizer_Test
	DeMap_Test
	Encoder_Test
	ErrorCode_Test
	Expected_Test
	GetArg_Test
	Invoker_Test
	Notifier_Test
	PacketProcessor_Test
	StackTraceExcept_Test
	Socket_Test
	Connector_Test
	BoostTimer_Test
	Redis_Test
)

FIND_PACKAGE ( hiredis REQUIRED )

FOREACH ( TEST IN LISTS TESTS )
	ADD_EXECUTABLE ( ${TEST} ${TEST}.cc)
	TARGET_LINK_LIBRARIES ( ${TEST} PRIVATE GTest::gtest_main GTest::gtest )
	ADD_TEST( NAME ${TEST} COMMAND ${TEST} )
ENDFOREACH()

TARGET_LINK_LIBRARIES ( ByteQueue_Test PRIVATE ByteQueue )
TARGET_LINK_LIBRARIES ( ConfigParser_Test PRIVATE ConfigParser )
TARGET_LINK_LIBRARIES ( Daemonizer_Test PRIVATE Daemonizer )
TARGET_LINK_LIBRARIES ( DeMap_Test PRIVATE DeMap )
TARGET_LINK_LIBRARIES ( Encoder_Test PRIVATE Encoder )
TARGET_LINK_LIBRARIES ( ErrorCode_Test PRIVATE ErrorCode )
TARGET_LINK_LIBRARIES ( Expected_Test PRIVATE Expected )
TARGET_LINK_LIBRARIES ( GetArg_Test PRIVATE GetArg )
TARGET_LINK_LIBRARIES ( Invoker_Test PRIVATE Invoker )
TARGET_LINK_LIBRARIES ( Notifier_Test PRIVATE Notifier )
TARGET_LINK_LIBRARIES ( PacketProcessor_Test PRIVATE PacketProcessor )
TARGET_LINK_LIBRARIES ( StackTraceExcept_Test PRIVATE StackTraceExcept )
TARGET_LINK_LIBRARIES ( Socket_Test PRIVATE MyTCPClient MyTCPServer MyTCPTLSClient MyTCPTLSServer MyWebsocketClient MyWebsocketServer MyWebsocketTLSClient MyWebsocketTLSServer pthread )
TARGET_LINK_LIBRARIES ( Connector_Test PRIVATE MyConnectorPool MyConnectee )
TARGET_LINK_LIBRARIES ( BoostTimer_Test PRIVATE pthread )
TARGET_LINK_LIBRARIES ( Redis_Test PRIVATE MyRedis )

UNSET ( TESTS )