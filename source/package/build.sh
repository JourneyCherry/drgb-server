#!/bin/sh

do_separate=0
do_verbose=0
do_clean=0
do_openrc=0
do_debugmode=0
do_showhowto=0
while getopts "svcodt:h" opt
do
	case $opt in
		(s) do_separate=1;;
		(v) do_verbose=1;;
		(c) do_clean=1;;
		(o) do_openrc=1;;
		(d) do_debugmode=1;;
		(h) do_showhowto=1;;
		(*)
			do_showhowto=1
			echo "Unknown Option : $opt"
		;;
	esac
done

shift $(($OPTIND - 1))

function_show_howto()
{
	echo "Usage : $0 [option] [target]";
	echo "Options";
	echo "	-s : separate packages per targets.";
	echo "	-v : verbose. actually does nothing.";
	echo "	-c : clean CPack folder and packages.";
	echo "	-o : openrc mode of Package";
	echo "	-d : debug mode of Package";
	echo "	-h : show usage";
	echo "Targets(Empty Target is same as \"auth match battle\")";
	echo "	auth : auth server";
	echo "	match : match server";
	echo "	battle : battle server";
	echo "	clean : clean CPack folder";
	echo "	help : show usage";
}

if [ $do_showhowto -ne 0 ]; then
	function_show_howto;
	exit 0;
fi

if [ $# -gt 0 ] && [ $1 = "help" ]; then
	function_show_howto;
	exit 0;
fi

if [ $do_verbose -ne 0 ]; then
	echo "Verbose Mode On"
fi

function_clean_by_product()
{
	echo "rm -rf ../build/*"
	if [ $do_verbose -eq 0 ]; then
		rm -rf ../build/*
	fi
	if [ -d "../build/.cmake" ]; then
		echo "rm -rf ../build/.cmake/"
		if [ $do_verbose -eq 0 ]; then
			rm -rf ../build/.cmake/
		fi
	fi
	echo "rm -rf ./_CPack_Packages"
	if [ $do_verbose -eq 0 ]; then
		rm -rf ./_CPack_Packages
	fi
}

function_clean()
{
	function_clean_by_product;
	echo "rm -rf drgb-*"
	if [ $do_verbose -eq 0 ]; then
		rm -rf drgb-*
	fi
}

build_hiredis()
{
	local target_file="hiredis-1.1.0-Linux.deb"
	if [ -f $target_file ]; then
		return
	fi

	set -e

	local build_dir="../../libraries/hiredis/build"
	local current_dir=`pwd`
	if [ ! -d $build_dir ]; then
		echo "mkdir $build_dir"
		if [ $do_verbose -eq 0 ]; then
			mkdir $build_dir
		fi
	fi
	echo "cd $build_dir"
	if [ $do_verbose -eq 0 ]; then
		cd $build_dir
	fi

	echo "Start make Hiredis"
	echo "cmake .. -cmake .. -DCMAKE_INSTALL_PREFIX=/usr"
	if [ $do_verbose -eq 0 ]; then
		cmake .. -DCMAKE_INSTALL_PREFIX=/usr
	fi
	echo "cpack -G DEB"
	if [ $do_verbose -eq 0 ]; then
		cpack -G DEB
	fi
	echo "mv ./$target_file $current_dir"
	if [ $do_verbose -eq 0 ]; then
		mv ./$target_file $current_dir
	fi

	echo "cd $current_dir"
	if [ $do_verbose -eq 0 ]; then
		cd $current_dir
	fi

	echo "rm -rf $build_dir"
	if [ $do_verbose -eq 0 ]; then
		rm -rf $build_dir
	fi
}


if [ $do_clean -ne 0 ]; then
	echo "Cleaning...";
	function_clean
	exit 0;
fi
if [ $# -gt 0 ] && [ $1 = "clean" ]; then
	echo "Cleaning...";
	function_clean
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
	build_hiredis

	local current_dir=`pwd`
	local build_dir=../build

	if [ ! -d $build_dir ]; then
		mkdir $build_dir
	fi

	echo "cd $build_dir"
	if [ $do_verbose -eq 0 ]; then
		cd $build_dir
	fi
	set -e
	echo "Start make Packages \"$@\""
	local openrc=""
	if [ $do_openrc -ne 0 ]; then
		openrc="-DSYSTEM_SERVICE_NAME=openrc"
	fi
	if [ $do_debugmode -ne 0 ]; then
		build_type="Debug"
	else
		build_type="Release"
	fi
	echo "cmake .. -DBUILD_TARGET=$(echo $@ | sed -e "s/ /;/g") -DCMAKE_BUILD_TYPE=$build_type $openrc"
	if [ $do_verbose -eq 0 ]; then
		cmake .. -DBUILD_TARGET=$(echo $@ | sed -e "s/ /;/g") -DCMAKE_BUILD_TYPE=$build_type $openrc
	fi
	echo "make -j $((`nproc` + 2))"
	if [ $do_verbose -eq 0 ]; then
		make -j $((`nproc` + 2))
	fi
	echo "cpack"
	if [ $do_verbose -eq 0 ]; then
		cpack
	fi
	echo "cd $current_dir"
	if [ $do_verbose -eq 0 ]; then
		cd $current_dir
	fi
}

if [ $do_separate -ne 0 ]; then
	for TG in $TARGET;
	do
		build_target $TG
		if [ $? -ne 0 ]; then exit 1; fi
	done
else
	build_target $TARGET
fi
