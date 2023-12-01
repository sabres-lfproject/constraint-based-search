FROM golang:bullseye as build

RUN apt update && apt upgrade -qy
RUN apt install -y \
    build-essential \
    cmake \
    make \
    ca-certificates \
    protobuf-compiler \
    gcc \
    g++ \
    libboost-all-dev  \
    golang

COPY . /cbs
WORKDIR /cbs

RUN rm -rf CMakeCache.txt CMakeFiles/
RUN cmake .
RUN make

WORKDIR /cbs/service

RUN make

FROM ubuntu:22.04 as final

RUN apt update && apt install -qy \
    iproute2 \
    vim

COPY --from=build /cbs/cbs /usr/bin/
COPY --from=build /cbs/service/service /usr/bin/cbs-service

RUN mkdir /data

COPY --from=build /cbs/service/pkg /data
    
ENV CBSPATH "/usr/bin/cbs"
ENV CBSHOST localhost
ENV CBSPORT 15030
ENV DATADIR "/data"
ENV DEBUG   "true"

# orchestratorctld grpc
EXPOSE ${CBSPORT}

CMD /usr/bin/cbs-service
