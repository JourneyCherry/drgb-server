# drgb-server

drgb server program

# How to Build

## Build with Linux

0. Install Required Libraries

	**Required Libraries for Build**
	|Library name|Minimum Version|
	|-|-|
	|cmake|3.18|
	|make|4.3-4.1|
	|g++|4:10.2.1-1|
	|libssl-dev|3.0.2|
	|libcrypto++-dev|8.4.0|
	|libboost-dev|1.74.0|
	|libpqxx-dev|6.4.5|
	|hiredis|1.1.0|
	|protobuf-compiler-grpc|1.30.2|
	|libprotobuf-dev|3.12.4|


	**Required Libraries for Package Installation**
	|Library name|Minimum Version|
	|-|-|
	|hiredis|1.1.0|
	|libgrpc++-dev|1.30.2|
	|libprotobuf-dev|3.12.4|
	|libpqxx-dev|6.4.5|

	**Required Libraries for Execution**
	- syslog-ng

1. Make package file

	``` bash
	cd /drgb-server/source/package
	./build.sh
	#if you want to make openrc service, add '-o' option to 'build.sh'
	```

2. Install required libraries into Linux machine

	``` bash
	#In Target Machine
	dpkg -i hiredis-1.1.0-Linux.deb	#debian package of hiredis will be made by './build.sh' on previous part.
	#If you need 'System Daemon' such as 'openrc' or 'init', install it too.
	```

3. Install package into the machine

	``` bash
	#In Target Machine
	dpkg -i drgb-all-0.1.0-Linux.deb
	```

4. Register Service

	``` bash
	#In Target Machine
	#target : auth match battle

	#For System Daemon
	systemctl enable syslog-ng
	systemctl enable drgb-<target>

	#For OpenRC
	mkdir /run/openrc
	touch /run/openrc/softlevel
	rc-update add syslog-ng
	rc-update add drgb-<target>
	```

5. Start Service

	``` bash
	service syslog-ng start
	service drgb-<target> start
	```

## Build with Docker

1. Make package file

	``` bash
	cd /drgb-server/source/package
	./build.sh
	```

2. Make Environmental Docker Image

	``` bash
	cd /drgb-server
	docker build -t drgb-env -f ./docker/Dockerfile-Env .
	```

3. Set up Services

	``` bash
	cd /drgb-server/docker
	docker-compose up --build -d
	```

4. When it finished, Tear down Services

	``` bash
	cd /drgb-server/docker
	docker-compose down --rmi local -v
	```