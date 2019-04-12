CPP_FILES = \
spicetrade/main.cpp \
spicetrade/mem.cpp

OUTPUT = spicetrade.exe

all:
	echo -------------------------------------------------
	g++ -std=gnu++0x $(CPP_FILES) -o $(OUTPUT)
	./spicetrade.exe
