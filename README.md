# unwave

convert Mutable Instruments .wav update file back into .bin file

usage:

1) clone directly into Mutable Instruments "eurorack" repository (unwave depends on some files from there):
	
	cd eurorack
	
	git clone https://github.com/av500/unwave.git
	
2) build with make, needs libsndfile
3) ./unwave ../build/clouds/clouds.wav clouds.bin

restrictions:

works with QPSK encoded files only for now, so won't work with e.g. Peaks

have fun :)
