EXE = avs_server
OBJS = main.o lwa.o config.o
LIBS = -ljson-c -lcurl

#CC = gcc
CFLAGS = -g 
#CFLAGS += -DHARDCODE_CONFIG

ALLOWED_INCLUDE_PATHS = -I/usr/local/include/json-c \
			-I/usr/local/include/curl \
			-I/usr/include/json-c \
			-I/usr/include/curl
ALLOWED_LIB_PATHS = -L/usr/local/lib/:/usr/lib/

.PHONY: all clean install

all: $(EXE)

$(EXE): $(OBJS) 
	$(CC) $^ -o $@ -L $(ALLOWED_LIB_PATHS) $(LIBS) 


main.o: main.c  
	$(CC) -c $(CFLAGS) main.c -o main.o $(ALLOWED_INCLUDE_PATHS)
lwa.o: lwa.c
	$(CC) -c $(CFLAGS) lwa.c -o lwa.o $(ALLOWED_INCLUDE_PATHS)
config.o: config.c
	$(CC) -c $(CFLAGS) config.c -o config.o $(ALLOWED_INCLUDE_PATHS)
install:
	cp $(EXE) /usr/local/bin/
	
clean:
	rm $(OBJS)
	rm $(EXE)

	


