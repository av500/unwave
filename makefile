CFLAGS = -I../ -lsndfile

SRC  = main.cc 
SRC += ../stm_audio_bootloader/qpsk/demodulator.cc
SRC += ../stm_audio_bootloader/qpsk/packet_decoder.cc

TARGET = unwave

all: $(TARGET)

$(TARGET): $(SRC) 
	g++ $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f *.o $(TARGET)  



