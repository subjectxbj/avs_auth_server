EXE = avs_server
OBJS = main.o lwa.o
LIBS = -lcjson -lcurl

CC = gcc
CFLAGS = -g 


ALLOWED_INCLUDE_PATHS = ./cJSON 
ALLOWED_LIB_PATHS = ./cJSON  

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJS) 
	$(CC) $^ -o $@ -L$(ALLOWED_LIB_PATHS) $(LIBS) 


main.o: main.c  
	$(CC) -c $(CFLAGS) main.c -o main.o -I$(ALLOWED_INCLUDE_PATHS)
lwa.o: lwa.c
	$(CC) -c $(CFLAGS) lwa.c -o lwa.o -I$(ALLOWED_INCLUDE_PATHS)

	


