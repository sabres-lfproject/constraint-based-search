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
    libboost-all-dev 

COPY . /cbs
WORKDIR /cbs

RUN cmake .
RUN make

FROM ubuntu:22.04 as final

RUN apt update && apt install -qy \
    iproute2 \
    vim

COPY --from=build /cbs/cbs /usr/bin/

RUN mkdir /data

COPY --from=build /cbs/ /data
    
ENV ETCDPORT 2379
ENV ETCDHOST localhost
ENV INVENTORYHOST localhost
ENV INVENTORYPORT 15005

# orchestratorctld grpc
EXPOSE ${CBSPORT}

CMD /usr/bin/cbs-wrapper
