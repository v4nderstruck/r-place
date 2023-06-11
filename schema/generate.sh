#!/bin/sh

flatc --cpp -o ../src protocol.fbs
flatc --python -o ../scripts protocol.fbs
