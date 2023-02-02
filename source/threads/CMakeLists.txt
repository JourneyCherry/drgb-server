SET ( LIBS
	Pool
	#FixedPool
	Thread
)

FOREACH( LIB IN LISTS LIBS )
	ADD_LIBRARY( ${LIB} ${LIB}.cpp ${LIB}.hpp )
ENDFOREACH()

TARGET_LINK_LIBRARIES ( Pool PUBLIC pthread MyExcepts )
TARGET_LINK_LIBRARIES ( Thread PUBLIC pthread MyExcepts )