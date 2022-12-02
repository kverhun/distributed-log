FROM ubuntu:20.04

# to supress interactive cmake installation
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y git
RUN apt-get update && apt-get install -y curl
RUN apt-get update && apt-get install -y cmake
RUN apt-get update && apt-get install -y gcc
RUN apt-get update && apt-get install -y g++

WORKDIR /
RUN git clone https://github.com/kverhun/distributed-log.git
WORKDIR distributed-log
RUN git fetch origin v2-write-concerns && git checkout v2-write-concerns

RUN mkdir cmake-build
WORKDIR cmake-build
RUN cmake ../ && cmake --build .

WORKDIR /
RUN ln -s /distributed-log/cmake-build/src/ReplicatedLogApp/ReplicatedLog ReplicatedLog
