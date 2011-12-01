CXXFLAGS =	-O2 -g -Wall -fmessage-length=0

OBJS =		netex1.o

LIBS =

TARGET =	netex1

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
