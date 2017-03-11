# SuperJX superjxsyxtool

## What This Program Does
This C++ command-line program manipulates system exclusive dumps from
MKS-70 and JX-10 synthesizers with ROM versions 1.08 through Vecoven 3.x.
HTML Program List output from the Nord Sound Manager program.
The reference card is a grid based on page and program. 
For keyboards with more than one bank of sounds, a separate grid is produced for each bank. 
It works for the Electro 4, Stage 2, and Lead A1 keyboards, at least.
This has been tested with the output of Nord Sound Manager 6.86 build 734_12 [OSX Intel].

## Why This Script
I wanted to rearrange patches in my MKS-70 and select patches and tones from various bulk dump
files. I had a hard time finding software that would do what I wanted. I think SoundDiver used
to do this, but I can't get it to run any more.

## What You Need
You need system exclusive dump from your MKS-70 or JX-10, or one of those
found elsewhere online. You need a C++ compiler. I am building on macOS with libc++
but there is not a lot of C++11 specific code here and you could probably find it 
and remove the C++11 dependency.

## Help
Here is the output of `superjxsyxtool -h` :
```
usage: superjxsyxtool [-h] [-p] [-u] [-U] [-f] [-x patch1,patch2,tone1,tone2,...]
                      [-s COPYFROMFILE [-c src1,dst1,src2,dst2,...]] [-o OUTPUTFILE] INPUTFILE

Manipulates Roland Super JX (MKS-70 / JX-10) system exclusive (syx) files.

positional arguments:
  INPUTFILE                           the input syx file

optional arguments:
  -h                                  show this help message and exit
  -o OUTPUTFILE                       write changed syx to OUTPUTFILE
  -p                                  print all patches and tones
  -u                                  print tones used by patches
  -U                                  print unused tones
  -v                                  include factory preset tones in printed output
  -x patch1,patch2,tone1,tone2,...    exchange patch1 from INPUTFILE with patch2, tone1 with
                                      tone2, and so on (requires pairs of patch/tone addresses),
                                      updating patches that use exchanged tones
  -X patch1,patch2,tone1,tone2,...    exchange patch1 from INPUTFILE with patch2, tone1 with
                                      tone2, and so on (requires pairs of patch/tone addresses),
                                      *without* updating patches that use exchanged tones
  -s COPYFROMFILE                     use COPYFROMFILE syx as the source for a -c operation
  -c src1,dst1,src2,dst2,...          copy src1 from COPYFROMFILE to dst1, src2 to dst1,
                                      and so on (requires pairs of patch/tone addresses)
  -f                                  force copying patches even if destination user tones are
                                      in use by another patch
```

## Examples
To print all patches and tones including factory preset tones from the bulk dump file dump.syx:
```
superjxsyxtool -p -f dump.syx
```

To exchange patches A2 and B1, and A3 and C1, and tones 21 and 22,
updating any patches that use tones 21 and 22,
and write the manipulated system exclusive dump to file newdump.syx:
```
superjxsyxtool -x A2,B1,21,22,A3,C1 -o newdump.syx dump.syx
```
