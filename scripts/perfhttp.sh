#!/bin/sh

wrk -t4 -c128 -d10s http://localhost:8081 -s perfhttp.lua
