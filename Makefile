CCOMPILE = gcc
CPPCOMPILE = g++
COMPILEOPTION = -c -g -fPIC -D__USERWLOCK -D__TEMP_COPY_ #-D_FILE_OFFSET_BITS=64 #-D__TEST_VERSION
INCLUDEDIR = -I /usr/local/include/gq/
LINK = g++
LIBDIRS =  -L /usr/local/lib
COMMONOBJS = 
SHAREDLIB = -lpthread -ldl -lgq -lhiredis
SHARE_SO=
APPENDLIB = 
SRCS	=	$(wildcard *.cpp)
OBJS	=	$(patsubst %.cpp, %.o, $(SRCS))   
TARGET = spider  
 
default:$(TARGET)

$(TARGET): $(OBJS)
	$(CPPCOMPILE) -g -o  $@ $^ $(LIBDIRS) $(SHAREDLIB)
.cpp.o: $(COMMONOBJS)
	$(CPPCOMPILE) -c -o $*.o $(COMPILEOPTION) $(INCLUDEDIR)  $*.cpp
	
.PHONY:clean
clean: 
	rm -f $(OBJS) $(TARGET)
	
	


	



	
	
