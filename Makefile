RACK_DIR ?= ../Rack-SDK

SOURCES += src/plugin.cpp
SOURCES += src/GinaArp.cpp

DISTRIBUTABLES += res

include $(RACK_DIR)/plugin.mk
