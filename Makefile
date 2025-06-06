CC = gcc
SRC = main.c tinyosc.c globmatch.c osc_handlers.c
INC = tinyosc.h osc_config.h
BIN = osc_firmware

$(BIN): Makefile $(SRC) $(INC)
	$(CC) -Wall -Werror -O0 -g -o $(BIN) $(SRC)

clean: 
	rm -f $(BIN)
