#! /bin/bash

trash dist
trash plugins
set -e
RACK_DIR=~/Sources/Rack make dist

mkdir -p /Applications/Rack.app/Contents/Resources/plugins
cp -r dist/RJModules /Applications/Rack.app/Contents/Resources/plugins/
mkdir -p ~/Documents/Rack/plugins-v1/RJModules
cp -r dist/RJModules ~/Documents/Rack/plugins-v1/
mkdir -p plugins
cp -r dist/RJModules plugins/
/Applications/Rack.app/Contents/MacOS/Rack -d
