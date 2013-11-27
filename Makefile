
TARGET	= webserver
OBJECTS	= main.o threadpool.o mylib.o serverlib.o
CC		= gcc -g
LIBS	= -lpthread

$(TARGET) : $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LIBS)

main.o:			mylib.h serverlib.h threadpool.h
mylib.o:		mylib.h
serverlib.o:	mylib.h serverlib.h
threadpool.o:	mylib.h threadpool.h

clean:
	rm $(OBJECTS)

