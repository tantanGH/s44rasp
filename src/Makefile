TARGET = s44rasp

CC = gcc
LD = gcc

CFLAGS = -O2
LDFLAGS =
LIBS = -lasound -lm

OBJS = adpcm_decode.o raw_decode.o wav_decode.o main.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	-rm -f *.o $(TARGET)