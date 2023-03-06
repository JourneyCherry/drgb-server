SET ( LIBS
	VariadicPool
	FixedPool
	Thread
	ThreadExceptHandler
)

FOREACH( LIB IN LISTS LIBS )
	IF ( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${LIB}.cpp" )
		ADD_LIBRARY( ${LIB} ${LIB}.cpp ${LIB}.hpp )
	ELSE()
		ADD_LIBRARY( ${LIB} INTERFACE ${LIB}.hpp )
	ENDIF()
ENDFOREACH()

TARGET_LINK_LIBRARIES ( FixedPool INTERFACE pthread ThreadExceptHandler StackTraceExcept )
TARGET_LINK_LIBRARIES ( VariadicPool INTERFACE pthread Expected StackTraceExcept )
TARGET_LINK_LIBRARIES ( Thread PUBLIC pthread ThreadExceptHandler )
TARGET_LINK_LIBRARIES ( ThreadExceptHandler PUBLIC pthread StackTraceExcept )