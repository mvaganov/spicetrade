EXE=spicetrade.exe
CC=g++
CFLAGS= -std=gnu++11
LDFLAGS= -lws2_32 
SRC_PATH=spicetrade/
BIN_PATH=bin/
INCL=-I $(SRC_PATH)

# find every .cpp file in src/
SRC=$(wildcard $(SRC_PATH)*.cpp)
HEADERS=$(wildcard $(SRC_PATH)*.h)
OBJ=$(subst spicetrade, bin, $(SRC:.cpp=.o))

ifeq ($(OS),Windows_NT)
	CLEANCMD=del bin\*.o
else
	CLEANCMD=rm -f $(OBJ)*.o *~
endif

# every .o file in $(BIN_PATH) depends on it's associated .cpp file $(SRC_PATH), and all .h files
$(BIN_PATH)%.o: $(SRC_PATH)%.cpp $(HEADERS)
# compile the ${@target} into the ${<output}
	$(CC) $(CFLAGS) $(INCL) -o "$@" "$<" -c

all: $(SRC) $(EXE)
	g++ -std=gnu++11 $(OBJ) -o $(EXE)
	$(BIN_PATH)$(EXE)

$(EXE): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $(BIN_PATH)$@

# prevent make from doing anything with a file named 'clean'
.PHONY: clean

clean:
	$(CLEANCMD)

# test network code
net:
	g++ spicetrade/echo.cp -std=gnu++0x -lws2_32 -o bin/nettest.exe
	bin/nettest.exe

