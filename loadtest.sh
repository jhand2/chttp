#!/bin/bash

for i in {1..100000}; do
    echo $i
    telnet localhost 8080
done
