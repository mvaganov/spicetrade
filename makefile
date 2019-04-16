CPP_FILES = \
spicetrade/main.cpp \
spicetrade/mem.cpp \
spicetrade/cli.cpp \
spicetrade/cards.cpp

OUTPUT = spicetrade.exe

all:
	echo -------------------------------------------------
	g++ -std=gnu++11 $(CPP_FILES) -o $(OUTPUT)
	./spicetrade.exe
