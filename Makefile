RACK_DIR ?= ../Rack-SDK

SOURCES += src/plugin.cpp
SOURCES += src/GinaArp.cpp
SOURCES += src/GinaArpCore.cpp
SOURCES += src/GinaArpScales.cpp
SOURCES += src/GinaArpRandom.cpp

DISTRIBUTABLES += res

include $(RACK_DIR)/plugin.mk
