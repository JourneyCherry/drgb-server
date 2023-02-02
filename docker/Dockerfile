FROM ubuntu

ARG TARGET
ARG SOURCE_DIR=../source
ARG WORK_DIR=/drgb-server

#Ubuntu package는 apt-get update를 해주어야 패키지 목록이 업데이트되어 타 패키지를 받을 수 있다.
RUN apt-get update
#-y 옵션은 yes 선입력이다.
RUN apt-get install -y \
	cmake \
	make \
	g++ \
	git \
	libtbb-dev \
	libboost-dev \
	#우분투는 기본 사양. 대신 /sbin/init이 없으므로 init을 받아야 한다. /sbin/init -> /usr/lib/systemd로 자동 링킹된다.
	#openrc \	
	init \
	syslog-ng \
	libpqxx-dev\
	#For Debug
	gdb \
	#musl-dbg \
	vim

#local5를 f_messages를 거치지 않게 변경
RUN sed -i -e "/                    not facility(auth,authpriv,cron,daemon,mail,news); };/c\\                    not facility(auth,authpriv,cron,daemon,mail,news,local5); };" /etc/syslog-ng/syslog-ng.conf

#drgb server Install
RUN mkdir ${WORK_DIR}
COPY ${SOURCE_DIR} ${WORK_DIR}
RUN if test -e ${WORK_DIR}/build; then rm -rf ${WORK_DIR}/build/*; else mkdir ${WORK_DIR}/build; fi
WORKDIR ${WORK_DIR}/build
RUN cmake .. -DBUILD_TARGET=${TARGET} -DCMAKE_BUILD_TYPE=Release
RUN make
RUN make install

RUN make clean
RUN cmake .. -DBUILD_TARGET=${TARGET} -DCMAKE_BUILD_TYPE=Debug
RUN make

#Windows 환경 docker에서 실행하는 경우, Carriage Return을 openrc가 인식하지 못하므로 제거해야함. git이 eol을 OS에 맞게 변경하므로, build할 때 변경을 해주는 것이 편하다.
RUN sed -i -e 's/\r//g' /etc/init.d/drgb-${TARGET}

#System Daemon Service Install
#RUN mkdir /run/openrc
#RUN touch /run/openrc/softlevel
#RUN rc-update add syslog-ng
#RUN rc-update add drgb-${TARGET}
#RUN service syslog-ng start
#RUN service drgb-${TARGET} start
RUN systemctl enable syslog-ng
RUN systemctl enable drgb-${TARGET}

ENTRYPOINT ["/sbin/init"]