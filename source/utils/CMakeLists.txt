SET ( LIBS
	ErrorCode
	Encoder
	ByteQueue
	#MyCodes
	ConfigParser
	Hasher
	Daemonizer
	#DeMap
	StackTraceExcept
	#Expected
	GetArg
	#Invoker
	Logger
	PacketProcessor
	Notifier
	#MyTypes
)

FOREACH( LIB IN LISTS LIBS )
	ADD_LIBRARY( ${LIB} ${LIB}.cpp ${LIB}.hpp )
ENDFOREACH()

TARGET_LINK_LIBRARIES ( Hasher PUBLIC crypto )
TARGET_LINK_LIBRARIES ( GetArg PUBLIC StackTraceExcept )
TARGET_LINK_LIBRARIES ( StackTraceExcept PUBLIC ErrorCode )