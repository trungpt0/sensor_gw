# Sensor Gateway

## Introduction about Sensor Monitoring System

The sensor monitoring system consists of sensor nodes measuring the room environment, a
sensor gateway that acquires all sensor data from the sensor nodes, and an SQL database to
store all sensor data processed by the sensor gateway. A sensor node uses TCP
connection to transfer the sensor data to the sensor gateway.

## Description

- The sensor gateway consists of a main process and a log process.
- The main process runs three threads: the connection, the data, and the storage
manager thread.
- The connection manager listens on a TCP socket for incoming connection requests
from new sensor nodes.
- The connection manager captures incoming packets of sensor nodes and writes the data to the shared data structure.
- The data manager thread reads sensor measurements from shared data.
- The log process receives log-events from the main process using a FIFO called
“logFifo”. If this FIFO doesn’t exists at startup of the main or log process.

## Technologies used

- C Programming
- Linux Programming: thread, fifo, mutex, file i/o, socket
