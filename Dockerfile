FROM ubuntu:20.04

# to supress interactive cmake installation
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y git
RUN apt-get update && apt-get install -y curl
RUN apt-get update && apt-get install -y cmake

RUN git clone https://github.com/kverhun/distributed-log.git
WORKDIR ./distributed-log
RUN git checkout poc
RUN mkdir cmake-build
WORKDIR cmake-build
RUN cmake ../ && cmake --build .
RUN APP_PATH=`pwd`/src/ReplicatedLogApp/ReplicatedLog

RUN echo "App path:"
RUN echo $APP_PATH
