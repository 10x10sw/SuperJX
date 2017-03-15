# SuperJX superjxsyxtool

## What This Program Does
This C++ command-line program manipulates system exclusive dumps from
MKS-70 and JX-10 synthesizers with ROM versions 1.08 through Vecoven 3.x.
I guess it's kind of a tough love librarian for the Super JX. 
This program generates a bulk dump file of patches and tones,
or simply prints the contents of the file.

## Why This Program
I wanted to rearrange patches in my MKS-70 and select patches and tones from various bulk dump
files. I wanted a automatic tone rearrangement when moving patches around. 
I had a hard time finding software that would do what I wanted. Maybe SoundDiver used
to do this, but I can't get it to run any more.

## What You Need
You need system exclusive from your MKS-70 or JX-10, either bulk dump or an individual patch dump.
You can generate this yourself by recording the bulk dump, or use one of the many files found elsewhere online.
To build this program, you need a C++ compiler. I am building on macOS with libc++.

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
superjxsyxtool -p -v dump.syx
```

To exchange patches A2 and B1, and A3 and C1, and tones 21 and 22,
updating any patches that use tones 21 and 22,
and write the manipulated system exclusive dump to file newdump.syx:
```
superjxsyxtool -x A2,B1,21,22,A3,C1 -o newdump.syx dump.syx
```

To copy patches c3 and d6 and their corresponding tones from jxsounds1.syx
to c1 and d1 in dump.syx and write the result to greatsounds.syx, and print the result:
```
superjxsyxtool -s jxsounds1.syx -c c3,c1,d6,d1 -o greatsounds.syx dump.syx
```

To copy the one tone from a single tone dump into tone 10 from bell.syx
and write the result to greatsounds.syx, and print the result:
```
superjxsyxtool -s bell.syx -f -c 1,10 -o greatsounds.syx greatsounds.syx
```
