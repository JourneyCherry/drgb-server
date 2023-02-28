SET ( TARGET match )
SET ( LIBS
	MyMatch
	MyMatchMaker
)

FOREACH ( LIB IN LISTS LIBS )
	ADD_LIBRARY ( ${LIB} ${LIB}.cpp ${LIB}.hpp )
ENDFOREACH()

ADD_EXECUTABLE ( ${TARGET} main.cpp )
SET_TARGET_PROPERTIES ( ${TARGET} PROPERTIES OUTPUT_NAME ${PROJECT_NAME}-${TARGET} )

TARGET_LINK_LIBRARIES ( MyMatchMaker PUBLIC
	
)

TARGET_LINK_LIBRARIES ( MyMatch PUBLIC
	MyServer
	MyConnectee
	MyConnector
	ConfigParser
	#MyCodes	#Header-Only
	MyMatchMaker
	Notifier
	MyPostgres
)

TARGET_LINK_LIBRARIES ( ${TARGET} PRIVATE
	MyMatch
	MyMain
)
UNSET ( TARGET )
UNSET ( LIBS )