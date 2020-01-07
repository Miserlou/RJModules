# FLAGS will be passed to both the C and C++ compiler
FLAGS += 
#FLAGS += -Idep/include
CFLAGS +=
CXXFLAGS +=

# Careful about linking to libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine.
LDFLAGS +=
#LDFLAGS += -lsndfile

# Add .cpp and .c files to the build
SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += $(wildcard LICENSE*) res soundfonts samples rawwaves

FLAGS += -w

#OBJECTS += $(libsndfile)
#DEPS += $(libsndfile)

#$(libsndfile):
#	cd dep/libsndfile make
#	cd dep/libsndfile make install 

RACK_DIR ?= ../..
include $(RACK_DIR)/plugin.mk
