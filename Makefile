RACK_DIR ?= ../Rack-SDK

SOURCES += src/plugin.cpp
SOURCES += src/GinaArp.cpp
SOURCES += src/GinaArpCore.cpp
SOURCES += src/GinaArpScales.cpp
SOURCES += src/GinaArpRandom.cpp
SOURCES += src/ProtoseqBlank.cpp
SOURCES += src/GinasExpander.cpp
SOURCES += src/Arc.cpp

DISTRIBUTABLES += res

TEST_CORE_EXE := build/gina_core_tests
TEST_ARC_EXE := build/arc_core_tests

.PHONY: test-core test-arc

test-core:
	@mkdir -p build
	$(CXX) -std=c++17 -Wall -Wextra -pedantic \
		tests/GinaArpCoreTests.cpp \
		src/GinaArpCore.cpp src/GinaArpScales.cpp src/GinaArpRandom.cpp \
		-o $(TEST_CORE_EXE)
	@$(TEST_CORE_EXE)

test-arc:
	@mkdir -p build
	$(CXX) -std=c++17 -Wall -Wextra -pedantic \
		tests/ArcCoreTests.cpp \
		-o $(TEST_ARC_EXE)
	@$(TEST_ARC_EXE)

ifeq ($(filter test-core test-arc,$(MAKECMDGOALS)),)
include $(RACK_DIR)/plugin.mk
endif
