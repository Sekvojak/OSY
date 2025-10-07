CC = g++
CFLAGS = -fPIC -Wall -Wextra -O2 -I..
LDFLAGS = -shared
LIB = libcheck.so

SRC = check_int.cpp
OBJ = $(SRC:.cpp=.o)

all: $(LIB)

%.o: %.cpp 
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB): $(OBJ)
	$(CC) $(LDFLAGS) -o $(LIB) $(OBJ)

clean:
	rm -f *.o $(LIB)
