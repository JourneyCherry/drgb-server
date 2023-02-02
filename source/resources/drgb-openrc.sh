#!/sbin/openrc-run
: ${project="drgb"}
: ${cfgfile="/etc/$project/$project.conf"}

pidfile="/var/run/$RC_SVCNAME.pid"
command="/usr/local/$project/$RC_SVCNAME"
command_args="
	--pidfile		$pidfile
	--configfile	$cfgfile
	--Log			default=5
	--daemon
	$command_args"

depend()
{
	use syslog-ng
}