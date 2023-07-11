FROM drgb-env

ARG MACHINE_ID=1
ARG DBADDR=db
ARG SESSIONADDR=session
ARG MATCHADDR=match
ARG TARGET

ARG DRGB_FILE_NAME=drgb-all-0.1.0-Linux.deb

WORKDIR /drgb-server

#Program Install
COPY ../source/package/${DRGB_FILE_NAME} /drgb-server/
RUN dpkg -i ${DRGB_FILE_NAME}
RUN rm ${DRGB_FILE_NAME}

#Set Configuration File
RUN sed -i \
	-e "0,/Machine_ID = 1/s//Machine_ID = ${MACHINE_ID}/" \
	-e "0,/DBAddr = localhost/s//DBAddr = ${DBADDR}/" \
	-e "0,/SessionAddr = localhost/s//SessionAddr = ${SESSIONADDR}/" \
	-e "0,/Match_Addr = localhost/s//Match_Addr = ${MATCHADDR}/" \
	/etc/drgb/drgb.conf

#Set Entrypoint for Docker
COPY ../source/resources/docker-entrypoint.sh /drgb-server/
RUN sed -i -e "s/drgb-all/drgb-${TARGET}/g" docker-entrypoint.sh
RUN chmod +x docker-entrypoint.sh

#Windows 환경 docker에서 실행하는 경우, Carriage Return을 bash가 인식하지 못하므로 제거해야함. git이 eol을 OS에 맞게 변경하므로, build할 때 변경을 해주는 것이 편하다.
RUN sed -i -e 's/\r//g' docker-entrypoint.sh

#ENTRYPOINT에는 ARG, ENV를 사용할 수 없다.
ENTRYPOINT [ "/drgb-server/docker-entrypoint.sh" ]	