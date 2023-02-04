SET ( LIBS
	MyBase64
	MyBytes
	#MyCodes
	MyConfigParser
	MyCrypto
	MyDaemoner
	#MyDeMap
	MyExcepts
	#MyExpected
	MyGetOpt
	#MyInvoker
	MyLogger
	MyMsg
	MyNotifier
	#MyTypes
)

FOREACH( LIB IN LISTS LIBS )
	ADD_LIBRARY( ${LIB} ${LIB}.cpp ${LIB}.hpp )
ENDFOREACH()

TARGET_LINK_LIBRARIES ( MyCrypto PUBLIC crypto )
TARGET_LINK_LIBRARIES ( MyGetOpt PUBLIC MyExcepts )
TARGET_LINK_LIBRARIES ( MyExcepts PUBLIC MyLogger )