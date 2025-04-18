# wadfs/makefile
CXX      := g++
CXXFLAGS := -O2 -Wall -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 \
            -I../libWad
LDFLAGS  := -lfuse -L../libWad -lWad

all: wadfs

wadfs: wadfs.cpp
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f wadfs wadfs.o
