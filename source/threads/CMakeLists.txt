SET ( LIBS
	FixedPool
	Thread
	ThreadExceptHandler
	VariadicPool
)

FOREACH( LIB IN LISTS LIBS )
	IF ( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${LIB}.cpp" )
		ADD_LIBRARY( ${LIB} EXCLUDE_FROM_ALL ${LIB}.cpp ${LIB}.hpp )
	ELSE()
		ADD_LIBRARY( ${LIB} INTERFACE EXCLUDE_FROM_ALL ${LIB}.hpp )
	ENDIF()
ENDFOREACH()

TARGET_LINK_LIBRARIES ( FixedPool INTERFACE ThreadExceptHandler StackTraceExcept )
TARGET_LINK_LIBRARIES ( Thread PUBLIC ThreadExceptHandler pthread )
TARGET_LINK_LIBRARIES ( ThreadExceptHandler PUBLIC )
TARGET_LINK_LIBRARIES ( VariadicPool INTERFACE Thread ThreadExceptHandler Expected StackTraceExcept )