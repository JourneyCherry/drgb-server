#!/bin/sh

do_separate=0
do_verbose=0
do_clean=0
do_openrc=0
while getopts "svco" opt
do
	case $opt in
		(s) do_separate=1;;
		(v) do_verbose=1;;
		(c) do_clean=1;;
		(o) do_openrc=1;;
		(*) echo "Unknown Option : $opt";;
	esac
done

shift $(($OPTIND - 1))

if [ $do_verbose -ne 0 ]; then
	echo "Verbose Mode On"
fi

function_clean_by_product()
{
	rm -rf ./_CPack_Packages;
}

function_clean()
{
	function_clean_by_product;
	rm -rf drgb-*;
}

build_hiredis()
{
	local target_file="hiredis-1.1.0-Linux.deb"
	if [ -f $target_file ]; then
		return
	fi

	local build_dir="../../libraries/hiredis/build"
	local current_dir=`pwd`
	if [ ! -d $build_dir ]; then
		mkdir $build_dir
	fi
	cd $build_dir

	echo "Start make Hiredis"
	cmake .. -DCMAKE_INSTALL_PREFIX=/usr
	cpack
	mv ./$target_file $current_dir

	cd $current_dir
	rm -rf $build_dir
}

if [ $# -eq 1 ] && [ $1 = "clean" ]; then
	if [ $do_verbose -eq 0 ]; then
		function_clean
	fi
	echo "Clean Outputs";
	exit 0;
elif [ $# -gt 0 ]; then 
	TARGET=$@;
else
	TARGET="auth match battle";
fi

echo "Target Packages : \"$TARGET\""

if [ $do_separate -ne 0 ]; then
	echo "Targets will build separately."
fi

build_target()
{
	local current_dir=`pwd`
	local build_dir=../build

	if [ ! -d $build_dir ]; then
		mkdir $build_dir
	fi

	cd $build_dir
	set -e
	echo "Start make Packages \"$@\""
	if [ $do_verbose -ne 0 ]; then
		return
	fi
	local openrc=""
	if [ $do_openrc -ne 0 ]; then
		openrc="-DSYSTEM_SERVICE_NAME=openrc"
	fi
	cmake .. -DBUILD_TARGET=$(echo $@ | sed -e "s/ /;/g") -DCMAKE_BUILD_TYPE=Release $openrc
	make -j $((`nproc` + 2))
	cpack
	cd $current_dir
}

build_hiredis

if [ $do_separate -ne 0 ]; then
	for TG in $TARGET;
	do
		build_target $TG
		if [ $? -ne 0 ]; then exit 1; fi
	done
else
	build_target $TARGET
fi

if [ $do_clean -ne 0 ]; then
	echo "Clear by-products."
	function_clean_by_product
fi