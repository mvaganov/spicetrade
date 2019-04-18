CPP_FILES = \
spicetrade/main.cpp \
spicetrade/mem.cpp \
spicetrade/cli.cpp \
spicetrade/cards.cpp

OUTPUT = spicetrade.exe

all:
	echo -------------------------------------------------
	g++ -std=gnu++11 $(CPP_FILES) -o $(OUTPUT)
	./$(OUTPUT)

net:
	echo testing network code
	g++ spicetrade/echo.cpp -std=gnu++0x -lws2_32 -o nettest.exe
	./nettest.exe
