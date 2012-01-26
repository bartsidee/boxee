CXX=g++
CC=g++

CFLAGS+=-Wall -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -O2
LDFLAGS+= -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4  -framework IOKit -framework Carbon 
LDFLAGS+= -weak_framework ForceFeedback

OBJS = iremoted.o AppleRemote.o

TARGET = boxeeservice
CLEAN_FILES=$(TARGET)

all: $(TARGET)

$(TARGET): $(OBJS)
	g++ $(LDFLAGS) $(OBJS) -o $(TARGET)

.cpp.o:
	$(CXX) -c $(CFLAGS) $(DEFINES) $(INCLUDES) $< -o ${<:.cpp=.o}

.c.o:
	$(CC) -c $(CFLAGS) $(DEFINES) $(INCLUDES) $< -o ${<:.c=.o}

clean:
	$(RM) -rf *.o XBMCHelper boxeeservice
