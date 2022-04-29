# wav-file-postprocessing
code to post-process existing .wav files

For now this is just a hack to learn about how sound is processed in Linux-accessible APIs.  My first foray was 
pulseaudio because I could not figure out Alsa (no good example and API documentation was not clear).
But I guess pulseaudio is being phased out in favor of pipewire, have no idea how that works.  Oh well... ;-)

Am planning to make a utility that just reads a .wav file, postprocesses effects onto it, then writes out a new .wav file with the results, which can then be played back using whatever tool you want to use.  Right now, effects are hardcoded inline, but want to make a pipeline of effects that can be layered upon one another like a guitar amp.  Also want to experiment with some things that aren't in a guitar amp because they are more computationally expensive.

package dependencies:

* pulseaudio-libs
* pulseaudio-libs-devel

To build:

# make

To debug:

# make OPT_FLAGS=-g

To run, see comments at top of source

This does not handle *all* wav file formats, only a small subset that are what I've come across, at least so far.

it does not yet handle 2 channels, with 2 channels we get half-speed playback, working on that.
