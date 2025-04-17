# MyMain/Makefile

CXX        := g++
CXXFLAGS   := -std=c++11 -Wall -Wextra -I../libWad

# The sources we need
MAIN_SRC   := main.cpp
WAD_SRC    := ../libWad/Wad.cpp

# Object files (basename only)
MAIN_OBJ   := main.o
WAD_OBJ    := Wad.o

# Final binary
TARGET     := main

.PHONY: all clean

all: $(TARGET)

# Link step
$(TARGET): $(MAIN_OBJ) $(WAD_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Compile your main
$(MAIN_OBJ): $(MAIN_SRC) ../libWad/Wad.h ../libWad/TreeNode.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile the WAD library source
$(WAD_OBJ): $(WAD_SRC) ../libWad/Wad.h ../libWad/TreeNode.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(MAIN_OBJ) $(WAD_OBJ) $(TARGET)
