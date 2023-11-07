#!/bin/bash

while true; do
    inotifywait -e modify main.py
    python3 main.py
done
