# Makefile for host-based unit tests
# Compiles Scribe editor component tests as standalone Linux binaries

CXX = g++
IDF_PATH ?= /home/niko/esp/esp-idf
CXXFLAGS = -std=c++17 -I../components/scribe_editor -I$(IDF_PATH)/components/unity/unity/src -I.
UNITY_SRC = $(IDF_PATH)/components/unity/unity/src/unity.c
EDITOR_SRCS = ../components/scribe_editor/piece_table.cpp ../components/scribe_editor/undo_stack.cpp
EDITOR_CORE_SRCS = $(EDITOR_SRCS) ../components/scribe_editor/editor_core.cpp ../components/scribe_editor/selection.cpp

.PHONY: all run clean test_piece_table test_undo_stack test_editor_core

all: test_piece_table test_undo_stack test_editor_core

test_piece_table: test_piece_table.cpp $(EDITOR_SRCS)
	$(CXX) $(CXXFLAGS) -DTEST_PIECE_TABLE -o $@ $^ $(UNITY_SRC)

test_undo_stack: test_undo_stack.cpp $(EDITOR_SRCS)
	$(CXX) $(CXXFLAGS) -DTEST_UNDO_STACK -o $@ $^ $(UNITY_SRC)

test_editor_core: test_editor_core.cpp $(EDITOR_CORE_SRCS)
	$(CXX) $(CXXFLAGS) -DTEST_EDITOR_CORE -o $@ $^ $(UNITY_SRC)

run: all
	./test_piece_table
	./test_undo_stack
	./test_editor_core

clean:
	rm -f test_piece_table test_undo_stack test_editor_core
