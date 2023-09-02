#!/bin/bash

svcname=drgb-all
exec="/usr/local/drgb/$svcname"

if [ $# -eq 0 ]; then
	#docker log를 사용하려면 std::cout을 사용해야 하므로, --visual 옵션을 준다.
	args="
			--configfile	/etc/drgb/$svcname.conf
			--pidfile		/var/run/$svcname.pid
			-v
		"
else
	args=$@
fi

exec $exec $args	#exec이 아닌 일반 실행은 자식 프로세스에서 실행하므로, 현재프로세스에서 실행하려면 exec으로 실행해야 한다.