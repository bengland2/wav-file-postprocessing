#!/bin/bash
set -eEo

# script to exercise readwav.c 
wav_file=${1:-short.wav}
suffix="_test_write"
new_file="$(basename $wav_file .wav)${suffix}.wav"
newer_file="$(basename $new_file .wav)${suffix}.wav"
rm -f $new_file $newer_file

export WAV_DEBUG=1 
echo original wav file
pacat $wav_file
sleep 1

./test_readwav $wav_file
echo copied to $new_file
pacat $new_file
sleep 1

# verify what we write is playable and parsable

./test_readwav $new_file
echo copied $new_file to $newer_file
pacat $newer_file
rm -f $new_file $newer_file
