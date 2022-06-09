#!/bin/bash
set -eEo

# script to exercise readwav.c 
wav_file=${1:-short.wav}
suffix="_test_write"
new_file="$(basename $wav_file .wav)${suffix}.wav"
newer_file="$(basename $new_file .wav)${suffix}.wav"
xform_file="$(basename $new_file .wav)_xform.wav"

function cleanup()
{
rm -f $new_file $newer_file $xform_file
}

cleanup

export WAV_DEBUG=1 
echo original wav file
pacat $wav_file
sleep 1

./copy_wav_file $wav_file $new_file
echo copied to $new_file
pacat $new_file
sleep 1

# verify what we write is playable and parsable

./copy_wav_file $new_file $newer_file
echo copied $new_file to $newer_file
pacat $newer_file

# transform sound of this file

./wav_transform -a 0.5 -m 100 -f 4000 -l -1.0 $wav_file $xform_file
echo transformed $wav_file to $xform_file
pacat $xform_file



