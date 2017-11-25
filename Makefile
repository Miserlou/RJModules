
SOURCES = $(wildcard src/*.cpp)


include ../../plugin.mk


dist: all
	mkdir -p dist/RJModules
	cp LICENSE* dist/RJModules/
	cp $(TARGET) dist/RJModules/
	cp -R res dist/RJModules/
