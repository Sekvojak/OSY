CXX = g++
CXXFLAGS = -Wall -Wextra -Iinc
LDFLAGS = -L./int_lib          
LDLIBS = -lcheck      

TARGET = vernum
SRC = vernum.cpp
OBJ = $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
