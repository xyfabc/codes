###########################################  
#Makefile for simple programs  
###########################################  
INC=-I./include/libusb-1.0  
LIB=-L./lib -lusb-1.0  
  
CC=mipsel-linux-gcc 
CC_FLAG=-Wall -Wl,rpath,  
  
PRG=card_reader  
OBJ= main.o
  
$(PRG):$(OBJ)  
	$(CC) $(INC) -o $@ $(OBJ) $(LIB)
	      
.SUFFIXES: .c .o .cpp  
.cpp.o:
	$(CC) $(CC_FLAG) $(INC) -c $*.cpp -o $*.o 
	  
.PRONY:clean  
clean:  
	@echo "Removing linked and compiled files......"  
	rm -f $(OBJ) $(PRG) 
