TARGET = s44rasp

CC = gcc
LD = gcc

CFLAGS = -O2 -Wall -D__USE_OLED__
LDFLAGS =
LIBS = -lasound -lm 

OBJS = adpcm_decode.o raw_decode.o wav_decode.o ym2608_decode.o ym2608_table.o macs_decode.o oled_ssd1306.o main.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	-rm -f *.o $(TARGET)