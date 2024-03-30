- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
The Apple Lossless Format
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

Apple Lossless supports the following features. Not all of these are implemented in alacconvert, though they are in the codec code provided.

1. Bit depths 16, 20, 24 and 32 bits.
2. Any arbitrary integer sample rate from 1 to 384,000 Hz. In theory rates up to 4,294,967,295 (2^32 - 1) Hz could be supported.
3. From one to eight channels are supported. Channel orders for the supported formats are described as:
	Num Chan	Order
	1 		mono
	2 		stereo (Left, Right)
	3 		MPEG 3.0 B (Center, Left, Right)
	4 		MPEG 4.0 B (Center, Left, Right, Center Surround)
	5 		MPEG 5.0 D (Center, Left, Right, Left Surround, Right Surround)
	6 		MPEG 5.1 D (Center, Left, Right, Left Surround, Right Surround, Low Frequency Effects)
	7 		Apple AAC 6.1 (Center, Left, Right, Left Surround, Right Surround, Center Surround, Low Frequency Effects)
	8 		MPEG 7.1 B (Center, Left Center, Right Center, Left, Right, Left Surround, Right Surround,  Low Frequency Effects)
4. Packet size defaults to 4096 sample frames of audio per packet. Other packet sizes are certainly possible. However, non-default packet sizes are not guaranteed to work properly on all hardware devices that support Apple Lossless. Packets above 16,384 sample frames are not supported.



- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
This package contains the sources for the Apple Lossless (ALAC) encoder and decoder.

The "codec" directory contains all the sources necessary for a functioning codec. Also includes is a makefile that will build libalac.a on a UNIX/Linux machine.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ALACconvert

The convert-utility directory contains sources to build alacconvert which is a simple utility that demonstrates how to use the included ALAC encoder and decoder.

alacconvert supports the following formats:

1. 16- or 24-bit mono or stereo .wav files where the data is little endian integer. Extended WAVE format chunks are not handled.
2. 16- or 24-bit mono or stereo .caf (Core Audio Format) files as well as certain multi-channel configurations where the data is big or little endian integer. It does no channel order manipulation.
3. ALAC .caf files.

Three project are provided to build a command line utility called alacconvert that converts cpm data to ALAC or vice versa. A Mac OS X Xcode project, A Windows Visual Studio project, and a generic UNIX/Linux make file.

Note: When building on Windows, if you are using a version of Visual Studio before Visual Studio 2010, <stdint.h> is not installed. You will need to acquire this file on your own. It can be put in the same directory as the project.



