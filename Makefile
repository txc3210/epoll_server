
OBJ_PATH = $(CUR_PATH)obj

CC = g++ -g -c 
CFLAGS = `mysql_config --cflags --libs` -luuid -I./inc -L./lib -lssl -lcrypto -ldl -lpthread

objects = main.o log.o

target = ./epoll_server

$(target):$(objects)
#	g++ -o $(target) *.o $(CFLAGS)
	g++ -o $(target) $(wildcard $(OBJ_PATH)/*.o) $(CFLAGS)
	
$(objects):%.o:%.cpp
	$(CC) $< -o $(OBJ_PATH)/$@ -lssl -lcrypto -ldl -lpthread
	
.PHONY : clean
clean :
	-rm $(target) $(wildcard $(OBJ_PATH)/*.o) $(wildcard *.o)
	
	

