#!/bin/bash

while true; do
    inotifywait -e modify main.c
    make test
done
