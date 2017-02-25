 Introduction
-------------
This package, based on ALSA lib, is used to test ALSA (audio) driver in A2/A3.
It can playback and capture audio in various audio format.

The package includes the following files:
        test_audio.cpp    -    The main application
        audio_encode.cpp  -    The Implementation of ITU-T(formerly CCITT)
                               Recomendation G711
        audio_encode.h    -    The header file for audio_encode.cpp
In fact, audio_encode.cpp and audio_encode.h is the partially source code of
Tixys-source, which is an open source code for encode/decode to/from various
audio format.

The application consist of 5 steps in capture mode:
(a) Open audio handle
(b) Set parameters to ALSA driver
(c) Capture 16-bit stereo PCM data from ALSA driver
(d) Cope with the 16-bit PCM data, eg. transforme it to A_LAW or MU_LAW. This
    step is implemented by Tixys-source code (audio_encode.cpp).
(e) Record the transformed audio data to local file.


 Compilation
------------
You should compile and "include" ALSA lib first before compile this package.


 Usage
------------
usage: test_audio [OPTION]...[FILE]...
        -h, --help              help
        -P, --playback          playback mode
        -C, --capture           capture mode
        -D, --duplex			duplex mode
        -c, --channels=#        channels
        -f, --format=FORMAT     sample format (case insensitive)
        -r, --rate=#            sample rate
        -s, --selch=#           select channel: 0 or 1

The default audio format that the application record to local file is
SND_PCM_FORMAT_A_LAW, 8K Hz and mono.

The program support PAUSE and RESUME when it is in playback mode.
You can press 'CTRL + Z' to PAUSE, and press 'CTRL + Z' again to RESUME.

To match your own requirment, you can re-define the following Macro in
test_audio.cpp and re-complie it.

        FILE_NAME :       The file name and path you want to record the audio
                          data.
        DEFAULT_FORMAT :  The sound format you want to record the audio data.
                          At present, the application just support 16-bit PCM,
                          MU_LAW and A_LAW, and only 16-bit PCM can be in
			  "stereo". If you want to record the audio data in
                          other format, you should "include" more source code
                          from Tixys-source.
        DEFAULT_RATE :    The sound sample rate, 8k, 44.1k ,48k and etc.
        DEFAULT_CHANNELS: The sound channel you want to record, mono or stereo ?


NOTE:   You should pay attention to the option of "aplay",if the option is
        incorrect, you may hear noise rather than the sound you record just now.

Following are some examples of options of "aplay"
        SND_PCM_FORMAT_S16_LE,48K,Stereo:aplay -fdat /mnt/file.dat
        SND_PCM_FORMAT_MU_LAW,8K, Stereo:aplay -f MU_LAW -r 8000 -c 2 /mnt/file.dat
        SND_PCM_FORMAT_A_LAW, 8K, Mono:  aplay -f A_LAW  -r 8000 -c 1 /mnt/file.dat


