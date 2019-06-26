#! /bin/bash

trash dist
trash plugins
set -e
RACK_DIR=~/Sources/Rack2/Rack make dist
