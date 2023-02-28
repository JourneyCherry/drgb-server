SET ( LIBS
	Pool
	#FixedPool
	Thread
	ThreadExceptHandler
)

FOREACH( LIB IN LISTS LIBS )
	ADD_LIBRARY( ${LIB} ${LIB}.cpp ${LIB}.hpp )
ENDFOREACH()

TARGET_LINK_LIBRARIES ( Pool PUBLIC pthread StackTraceExcept )
TARGET_LINK_LIBRARIES ( Thread PUBLIC pthread ThreadExceptHandler )
TARGET_LINK_LIBRARIES ( ThreadExceptHandler PUBLIC pthread StackTraceExcept )