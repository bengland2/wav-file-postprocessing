#!/bin/bash
# script to exercise readwav.c 
wav_file=short.wav
suffix="_test_write"

export WAV_DEBUG=1 
echo original wav file
pacat $wav_file
sleep 1

./test_readwav $wav_file
new_file="$(basename $wav_file .wav)${suffix}.wav"
echo copied to $new_file
pacat $new_file
sleep 1

# verify what we write is playable and parsable

new_file="$(basename $new_file .wav)${suffix}.wav"
echo copied to $new_file
pacat $new_file
sleep 1
