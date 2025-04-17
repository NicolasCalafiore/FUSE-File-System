CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -I.
AR       := ar
ARFLAGS  := rcs
TARGET   := libWad.a
SRCS     := Wad.cpp
OBJS     := $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

