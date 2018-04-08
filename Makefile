CC=gcc
SRC=cuckoo.c cuckoo_test.c
CGLAGS=-g

OBJS:=app

all:$(OBJS)

app: $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

clean: 
	rm -rf $(OBJS) 

