# http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
EXE=spicetrade.exe
CC=g++
# -g allows gdb debugging
CFLAGS= -std=gnu++11 -g
CFLAGS_RELEASE= -std=gnu++11
LDFLAGS= 
SRC_PATH=spicetrade/
BIN_PATH=bin/
INCL=-I $(SRC_PATH)

# find every .cpp file in src/
SRC=$(wildcard $(SRC_PATH)*.cpp)
HEADERS=$(wildcard $(SRC_PATH)*.h)
OBJ=$(subst spicetrade, bin, $(SRC:.cpp=.o))

ifeq ($(OS),Windows_NT)
	CLEANCMD=del bin\*.o
	DEBUGGER=gdb
else
	CLEANCMD=rm -f $(OBJ)*.o *~
	DEBUGGER=lldb
endif

# every .o file in $(BIN_PATH) depends on it's associated .cpp file $(SRC_PATH), and all .h files
$(BIN_PATH)%.o: $(SRC_PATH)%.cpp $(HEADERS)
# compile the ${@target} into the ${<output}
	$(CC) $(CFLAGS) $(INCL) -o "$@" "$<" -c

release: $(SRC) $(EXE)
	g++ $(CFLAGS_RELEASE) $(OBJ) -o $(EXE)
	$(BIN_PATH)$(EXE)

all: $(SRC) $(EXE)
	g++ $(CFLAGS) $(OBJ) -o $(EXE)
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

debug:
	$(DEBUGGER) spicetrade.exe
# 'b <name>', 'break <name>' set a breakpoint at function named <name>. eg: 'b main'
# 'r', 'run' will start the program, which you want to call after setting break points.
# 'f' show the code frame, where the debugger is
# 'l', 'list' continue listing code
# 'n', 'next' goes to the next line, executing it and stepping over it
# 's', 'step' goes into the current line, stepping into any subprocedures
# 'c', 'continue' continue execution
# 'finish' finishes executing the current stack frame, breaking on return.
# 'p <name>', 'print <name>' will print variable contents in the current frame, where <name> is a variable name. eg: 'print g' will print out the 'g' variable. works on objects!
# 'q', 'quit' quits debugging
# 'bt' show the call stack
# 'breakpoint list' lists current breakpoints
# 'help' gets more usage info about the debugger
# 'x' memory read... 'help x' for details