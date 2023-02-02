#!/bin/sh
### BEGIN INIT INFO
# Provides:				drgb
# Required-Start:		$local_fs $syslog syslog-ng
# Required-Stop:		$local_fs $syslog
# Default-Start:		2 3 4 5
# Default-Stop:			0 1 6
# Short-Description:	run drgb server
# Description:			init script to start drgb-server daemon
### END INIT INFO

project="drgb"
svcname=`basename $0 .sh`
cfgfile="/etc/$project/$project.conf"
pidfile="/var/run/$svcname.pid"
cmd="/usr/local/$project/$svcname"
cmd_args="
	--pidfile		$pidfile
	--configfile	$cfgfile
	--Log			default=5
	--daemon
"

start()
{
	if [ -f $pidfile ]; then
		echo "pid file exists, script is already running or crashed"
		exit 1
	fi
	echo "Starting $svcname"
	job="$cmd $cmd_args &"
	if command -v sudo >> /dev/null 2>&1; then
		sudo $job
	else
		$job
	fi
	#echo $! > $pidfile	#pidfile은 --pidfile 옵션이 있으면 프로그램이 직접 생성한다.
}

stop()
{
	if [ ! -f $pidfile ]; then
		echo "pid file not found, script not running"
		exit 1
	fi
	PID=$(cat $pidfile)
	echo "Stopping $svcname"
	job="kill $PID"
	if command -v sudo >> /dev/null 2>&1; then
		sudo $job
	else
		$job
	fi
	rm $pidfile
}

status()
{
	if [ test -e $pidfile ]; then
		if [ ps -p $(cat $pidfile) ]; then
			exit 0
		fi
	fi
	exit 1
}


case "$1" in
	start)
		start
		;;
	stop)
		stop
		;;
	restart)
		stop
		sleep 1
		start
		;;
	status)
		status
		;;
	*)
		echo "Usage: $0 {start|stop|restart|status}"
esac