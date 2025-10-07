CXX = g++
CXXFLAGS = -Wall -Wextra
AR = ar
ARFLAGS = rcs

TARGET = gennum
LIB = libgen.a
SRC = generate_line.cpp
OBJ = $(SRC:.cpp=.o)

all: $(TARGET)

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.cpp gen_line.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(TARGET).cpp $(LIB)
	$(CXX) $(CXXFLAGS) $< -L. -lgen -o $@

clean:
	rm -f *.o *.a $(TARGET)