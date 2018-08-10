#! /bin/bash

RACK_DIR=~/Downloads/Rack-SDK/ make dist
mkdir -p /Applications/Rack.app/Contents/Resources/plugins
trash /Applications/Rack.app/Contents/Resources/plugins/RJModules
cp -r dist/RJModules /Applications/Rack.app/Contents/Resources/plugins/RJModules
/Applications/Rack.app/Contents/MacOS/Rack -d
