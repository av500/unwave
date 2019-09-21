#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "sndfile.h"

#include "stm_audio_bootloader/qpsk/packet_decoder.h"
#include "stm_audio_bootloader/qpsk/demodulator.h"

using namespace stmlib;
using namespace stm_audio_bootloader;

const double kSampleRate     = 48000.0;
const double kModulationRate = 6000.0;
const double kBitRate        = 12000.0;

PacketDecoder decoder;
Demodulator demodulator;

void decode_mono(SNDFILE *fin, int fout);

int main(int argc, char **argv)
{
	if (argc < 3) {
		printf("unwave <infile.wav> <outfile.bin>\n");
		return -1;
	}

	SF_INFO info;
	memset(&info, 0, sizeof(SF_INFO));

	SNDFILE *fh = sf_open(argv[1], SFM_READ, &info);
	if (!fh) {
		printf("ERROR: cannot open file %s\n", argv[1]);
		return -1;
	}

	if((info.format & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16 ) {
		printf("not 16 bit!\n");
		sf_close(fh);
		return -1;
	}

	int fout = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU );
	if( fout == -1 ) {
		printf("cannot open outfile %s!\n", argv[2]);
		sf_close(fh);
		return -1;
	}
	
	if (info.channels == 1) {
		printf("decoding file\n");
		decode_mono( fh, fout );
	} else  {
		printf("BUMMER! mono only!\n");
		sf_close(fh);
		return -1;
	}

	// close files
	sf_close(fh);
	close(fout);
}

const uint32_t kBlockSize = 16384;
const uint16_t kPacketsPerBlock = kBlockSize / kPacketSize;
static uint8_t rx_buffer[kBlockSize];
static uint16_t packet_index;
static size_t current_address;

void decode_mono( SNDFILE * fin, int fout )
{
	decoder.Init( 40000 );
	demodulator.Init( kModulationRate / kSampleRate * 4294967296.0, kSampleRate / kModulationRate, 2.0 * kSampleRate / kBitRate );
	demodulator.SyncCarrier( true );
	decoder.Reset(  );

	bool done  = false;
	bool error = false;
	bool eof   = false;

	while ( !error && !done ) {
		short sample[32];
		int read = sf_read_short( fin, sample, 32 );
		if ( read == 0 ) {
			// reached end of file, have to flush last values
printf("EOF!\n");
			eof = true;
		} else {
			int pos = 0;
			while( pos < read ) {
				int32_t s32 = ( sample[pos++] >> 4 ) + 2048;
				demodulator.PushSample( s32 );
			}
		}
		
		if ( demodulator.state(  ) == DEMODULATOR_STATE_OVERFLOW ) {
printf("OVERFLOW err!\n");
			error = true;
		} else {
			demodulator.ProcessAtLeast( 32 );
		}

		while ( demodulator.available() && !error) {
			uint8_t symbol = demodulator.NextSymbol();
			PacketDecoderState state = decoder.ProcessSymbol( symbol );
			switch ( state ) {
			case PACKET_DECODER_STATE_OK:
				{
					memcpy( rx_buffer + ( packet_index % kPacketsPerBlock ) * kPacketSize, decoder.packet_data(), kPacketSize );
					++packet_index;
					if ( ( packet_index % kPacketsPerBlock ) == 0 ) {
printf("block %08d\n", current_address );
						write(fout, rx_buffer, kBlockSize );
						current_address += kBlockSize; 
						decoder.Reset();
						demodulator.SyncCarrier( false );
					} else {
						decoder.Reset();
						demodulator.SyncDecision();
					}
				}
				break;
			case PACKET_DECODER_STATE_ERROR_SYNC:
printf("SYNC err!\n");
				error = true;
				break;
			case PACKET_DECODER_STATE_ERROR_CRC:
printf("CRC err!\n");
				error = true;
				break;
			case PACKET_DECODER_STATE_END_OF_TRANSMISSION:
printf("END!\n");
				done = true;
				break;
			default:
				break;
			}
		}
		if( eof ) { 
			break;
		}
	}
printf("wrote %08d bytes\n", current_address );
}

