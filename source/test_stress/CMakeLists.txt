SET ( EXEC_NAME Stress_Test )

SET ( LIBS
	TestOpt
	TestClient
)

FOREACH ( LIB IN LISTS LIBS )
	ADD_LIBRARY ( ${LIB} EXCLUDE_FROM_ALL ${LIB}.cpp )
ENDFOREACH ()
ADD_EXECUTABLE ( ${EXEC_NAME} main.cpp )

TARGET_LINK_LIBRARIES ( ${EXEC_NAME} PUBLIC pthread TestOpt TestClient )
TARGET_LINK_LIBRARIES ( TestOpt PUBLIC GetArg StackTraceExcept )
TARGET_LINK_LIBRARIES ( TestClient PUBLIC ThreadExceptHandler MyWebsocketClient Hasher )

UNSET ( LIBS )