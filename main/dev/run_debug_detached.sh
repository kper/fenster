#!/usr/bin/env bash

# cd to project root
cd "$(dirname "$0")/.."

gtimeout 30s make debug &

sleep 3s