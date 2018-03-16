EXE = avs_server
OBJS = main.o lwa.o config.o
LIBS = -lcjson -lcurl

CC = gcc
CFLAGS = -g 


ALLOWED_INCLUDE_PATHS = -I./cJSON \
						-I/usr/include/arm-linux-gnueabihf/curl
ALLOWED_LIB_PATHS = ./cJSON

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJS) 
	$(CC) $^ -o $@ -L $(ALLOWED_LIB_PATHS) $(LIBS) 


main.o: main.c  
	$(CC) -c $(CFLAGS) main.c -o main.o $(ALLOWED_INCLUDE_PATHS)
lwa.o: lwa.c
	$(CC) -c $(CFLAGS) lwa.c -o lwa.o $(ALLOWED_INCLUDE_PATHS)
config.o: config.c
	$(CC) -c $(CFLAGS) config.c -o config.o $(ALLOWED_INCLUDE_PATHS)
	
clean:
	rm $(OBJS)
	rm $(EXE)

	


