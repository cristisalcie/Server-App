# Protocoale de comunicatii:
# Laborator 8: Multiplexare
# Makefile

CFLAGS = -Wall -g

# Portul pe care asculta serverul
PORT = 10025

# Adresa IP a serverului
IP_SERVER = 127.0.0.1

all: server subscriber

# Compileaza server.c
server: server.c server.h

# Compileaza subscriber.c
subscriber: subscriber.c

.PHONY: clean run_server run_subscriber

# Ruleaza serverul
run_server:
	valgrind --leak-check=full ./server ${PORT}

# Ruleaza subscriber
run_subscriber:
	valgrind --leak-check=full ./subscriber $(ID) ${IP_SERVER} ${PORT}

# Ruleaza udp
run_udp:
	python3 udp_client.py --mode manual $(IP_SERVER) $(PORT)

clean:
	rm -f server subscriber *.sf
