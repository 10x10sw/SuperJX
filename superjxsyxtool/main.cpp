//
//  main.cpp
//  superjxsyxtool
//
//  Created by Christian Erickson on 3/9/17.
//  Copyright © 2017 Ten by Ten. All rights reserved.
//

#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <vector>

namespace
{
	typedef std::vector<unsigned char> SyxBuffer;
	typedef std::vector<SyxBuffer> Patches;
	typedef std::vector<SyxBuffer> Tones;
	typedef std::set<unsigned char> ToneNumberSet;
	typedef std::pair<unsigned char, unsigned char> Exchange;
	typedef std::vector<Exchange> PatchSwaps;
	typedef std::vector<Exchange> ToneSwaps;
	typedef std::pair<PatchSwaps, ToneSwaps> Swaps;
	
	const char* presetTones[] = { "PIANO 1", "E. GRAND 1", "PIANO 2", "CELLO SECT", "ARCO STRING",
		"LOW STRINGS", "HI STRINGS", "BEE-THREE", "ORGAN 1", "CALIOPE",
		"PIPE ORGAN", "DRYSTLDRUM", "MUSIC BOX", "WINDCHIMES", "E. BASS",
		"SYNTH BASS", "SOUNDTRACK", "HOLLOW PAD", "FLUTE 1", "FRETNOT 1",
		"BIG OL PAD", "STABBRASS 2", "POLYSYNTH 2", "GOWESTBRS 2", "GOWESTBRS 1",
		"POLY BRASS", "GAMELANET", "CELESTE 2", "AGOGO BELL", "SYNDULCIMR",
		"GUITARCLAV", "PERKPIANO", "PIANO 4", "SYNC LEAD", "SEQ* 1",
		"RECORDERS", "BRIGHT BOWS", "STRINGS 1", "STRINGS 2", "CHOIR",
		"MAY,S WIND", "MARIMBA", "METALLET", "SYNTHBELL 2", "XMAS BELLS",
		"VIBES", "CHURCHBELL", "RES BELL", "KALIMBA 2", "GOWESTVOX", };

	static SyxBuffer ReadFile(const char* path)
	{
		std::ifstream file(path, std::ios::in | std::ios::binary);
		if (file.good())
		{
			SyxBuffer syx((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			return syx;
		}
		return SyxBuffer();
	}
	
	static void WriteFile(const char* path, const SyxBuffer& syx)
	{
		std::ofstream file(path, std::ios::out | std::ios::binary);
		if (file.good())
			file.write((const char*)&syx.at(0),syx.size());
	}
	
	static unsigned char CombineBytes(unsigned char top, unsigned char bot)
	{
		top = top << 4;
		if (top > 127) top -= 128;
		return top | bot;
	}
	
	static bool CompressPatch(SyxBuffer& syx, SyxBuffer::iterator& patchStart)
	{
		// take 96 bytes (9-105) and compress them into 48
		patchStart += 9;
		SyxBuffer compressedPatch(48,0);
		
		for (int i=0; i<48; ++i)
			compressedPatch[i] = CombineBytes(patchStart[i*2],patchStart[i*2+1]);
		
		patchStart = syx.insert(patchStart,compressedPatch.begin(),compressedPatch.end());
		patchStart += compressedPatch.size();
		patchStart = syx.erase(patchStart,patchStart+compressedPatch.size()*2);
		
		return true;
	}
	
	static bool FindPatch(const SyxBuffer& syx, SyxBuffer::const_iterator& from)
	{
		SyxBuffer::const_iterator patchStart = from;
		do
		{
			if ((patchStart != syx.end())
				&& (patchStart+105 < syx.end())
				&& (patchStart[0] == 0xf0)
				&& (patchStart[1] == 0x41) // Roland
				&& (patchStart[4] == 0x24) // MKS-70 / JX-10
				&& (patchStart[5] == 0x30) // Patch Data
				&& (patchStart[105] == 0xf7)) // end of patch data (106 bytes in file)
			{
				from = patchStart;
				return true;
			}
		}
		while (++patchStart != syx.end());
		return false;
	}
	
	static bool FindTone(const SyxBuffer& syx, SyxBuffer::const_iterator& from)
	{
		SyxBuffer::const_iterator toneStart = from;
		do
		{
			if ((toneStart != syx.end())
				&& (toneStart+68 < syx.end())
				&& (toneStart[0] == 0xf0)
				&& (toneStart[1] == 0x41) // Roland
				&& (toneStart[4] == 0x24) // MKS-70 / JX-10
				&& (toneStart[5] == 0x20) // Tone Data
				&& (toneStart[68] == 0xf7)) // end of tone data (69 bytes in file)
			{
				from = toneStart;
				return true;
			}
		}
		while (++toneStart != syx.end());
		return false;
	}
	
	Patches ParsePatches(const SyxBuffer& syx)
	{
		Patches patches;
		SyxBuffer::const_iterator it = syx.begin();
		for (int b=0; b<64; ++b)
		{
			if (FindPatch(syx,it))
			{
				SyxBuffer patch(it,it+106);
				it+=106;
				patches.push_back(patch);
			}
			else
				return Patches();
		}
		return patches;
	}
	
	Tones ParseTones(const SyxBuffer& syx)
	{
		Tones tones;
		SyxBuffer::const_iterator it = syx.begin();
		for (int i=0; i<50; ++i)
		{
			if (FindTone(syx,it))
			{
				SyxBuffer tone(it,it+69);
				it+=69;
				tones.push_back(tone);
			}
			else
				return Tones();
		}
		return tones;
	}
	
	void GetPatchTones(const SyxBuffer& patch, unsigned char& a, unsigned char& b)
	{
		a = CombineBytes(patch[9+64],patch[9+65]);
		b = CombineBytes(patch[9+54],patch[9+55]);
	}
	
    void SetPatchTones(SyxBuffer& patch, unsigned char a, unsigned char b)
    {
        patch[9+64] = a >> 4;
        patch[9+65] = a & 0xf;
        patch[9+54] = b >> 4;
        patch[9+55] = b & 0xf;
    }
    
	ToneNumberSet ParseUsedTones(const Patches& banks)
	{
		ToneNumberSet tones;
		for (Patches::const_iterator it=banks.begin(); it!=banks.end(); ++it)
		{
			const SyxBuffer& patch(*it);
			unsigned char a = 0, b = 0;
            GetPatchTones(patch,a,b);
            tones.insert(a);
            tones.insert(b);
        }
		return tones;
	}
	
	bool IsPatch(const std::string& str)
	{
		return ((str.size()==2)
			&& ((str[0] >= 'A' && str[0] <='H')
				|| (str[0] >= 'a' && str[0] <= 'h'))
			&& (str[1] >= '1' && str[1] <= '8'));
	}
	
	unsigned char ParsePatchNumber(const std::string& str)
	{
		if (IsPatch(str))
		{
			unsigned char b = str[0] > 'H' ? str[0]-'a' : str[0]-'A';
			unsigned char p = str[1] - '1';
			return b*8+p;
		}
		return 0;
	}
	
	unsigned char ParseToneNumber(const std::string& str)
	{
		return static_cast<unsigned char>(strtoul(str.c_str(),NULL,10)-1);
	}
	
	bool IsTone(const std::string& str)
	{
		unsigned char t = ParseToneNumber(str);
		return t>0 && t<51;
	}

	static Swaps ParseSwaps(const char* swapstr)
	{
		Swaps swaps;
		std::stringstream ss;
		ss.str(swapstr);
		std::string n1;
		while (std::getline(ss, n1, ','))
		{
			std::string n2;
			if (std::getline(ss, n2, ','))
			{
				if (IsPatch(n1) && IsPatch(n2))
				{
					Exchange exchange(ParsePatchNumber(n1),ParsePatchNumber(n2));
					swaps.first.push_back(exchange);
				}
				else if (IsTone(n1) && IsTone(n2))
				{
					Exchange exchange(ParseToneNumber(n1),ParseToneNumber(n2));
					swaps.second.push_back(exchange);
				}
				else
					break;
			}
			else
				break;
		}
 		return swaps;
	}
	
    static void SwapPatchTones(Patches& patches, unsigned char t1, unsigned char t2)
    {
        for (Patches::iterator it=patches.begin(); it!=patches.end(); ++it)
        {
            unsigned char a = 0, b = 0;
            GetPatchTones(*it,a,b);
            if (a == t1)
                a = t2;
            else if (a == t2)
                a = t1;
            if (b == t1)
                b = t2;
            else if (b == t2)
                b = t1;
            SetPatchTones(*it,a,b);
        }
    }
    
	static void SwapPatches(const PatchSwaps& swaps, Patches& patches)
	{
		if (!swaps.empty())
		{
			std::cerr << "Swapping patches:";
			for (PatchSwaps::const_iterator it=swaps.begin(); it!=swaps.end(); ++it)
			{
				unsigned char p1 = it->first;
				unsigned char p2 = it->second;
				std::cerr << " " << (char)(65+p1/8) << p1%8+1 << "<=>" << (char)(65+p2/8) << p2%8+1;
				Patches::iterator it1 = patches.begin()+p1;
				Patches::iterator it2 = patches.begin()+p2;
				std::swap(*it1,*it2);
			}
			std::cerr << std::endl;
		}
    }
    
    static void SwapTones(const ToneSwaps& swaps, Patches& patches, Tones& tones)
    {
		if (!swaps.empty())
		{
			std::cerr << "Swapping tones:";
			for (ToneSwaps::const_iterator it=swaps.begin(); it!=swaps.end(); ++it)
			{
                // when swapping tones, make sure the patches are updated too
				unsigned char t1 = it->first;
				unsigned char t2 = it->second;
                SwapPatchTones(patches,t1,t2);
				std::cerr << " " << (int)t1+1 << "<=>" << (int)(t2+1);
				Patches::iterator it1 = tones.begin()+t1;
				Patches::iterator it2 = tones.begin()+t2;
				std::swap(*it1,*it2);
			}
			std::cerr << std::endl;
		}
	}
	
    static void CopyPatchesAndTones(const Swaps& swaps, Patches& patches, Tones& tones)
    {
        
    }

	static SyxBuffer WriteSyx(Patches& patches, Tones& tones)
	{
		SyxBuffer syx;
		for (Patches::iterator it=patches.begin(); it!=patches.end(); ++it)
			syx.insert(syx.end(),it->begin(),it->end());
		for (Tones::iterator it=tones.begin(); it!=tones.end(); ++it)
			syx.insert(syx.end(),it->begin(),it->end());
		return syx;
	}
	
	static void CompressPatches(SyxBuffer& syx)
	{
		SyxBuffer::const_iterator cit = syx.begin();
		while (FindPatch(syx,cit))
		{
			SyxBuffer::iterator it = syx.begin() + (cit - syx.begin());
			CompressPatch(syx,it);
			cit = syx.begin() + (it - syx.begin());
		}
	}
	
	static void FixNameNumbers(std::string& name)
	{
		for (std::string::iterator it = name.begin(); it != name.end(); ++it)
		{
			if (*it < 10) *it += 48;
		}
	}
	
	static void PrintPatchesAndTones(const SyxBuffer& syx, bool printPresets)
	{
		SyxBuffer::const_iterator patch=syx.begin();
		std::cerr << "-----------------------------------" << std::endl;
		std::cerr << "PATCH      NAME              A    B" << std::endl;
		std::cerr << "-----------------------------------" << std::endl;
		for (int i=0; i<64; ++i)
		{
			std::cerr << " " << (char)(65+i/8) << i%8+1 << "  ";
			std::string patchName(patch+9,patch+27);
			FixNameNumbers(patchName);
			unsigned char a = *(patch+41) + 1;
			unsigned char b = *(patch+36) + 1;
			std::cerr << patchName << "    " << std::setfill(' ') << std::setw(3) << (int)a << "  " << std::setw(3) << (int)b << std::endl;
			if ((i+1)%8==0) std::cerr << std::endl;
			patch += 58;
		}
		std::cerr << "--------------" << std::endl;//------------------------------" << std::endl;
		std::cerr << "T#   TONE NAME" << std::endl;//              T#   TONE NAME  " << std::endl;
		std::cerr << "--------------" << std::endl;//------------------------------" << std::endl;
		SyxBuffer::const_iterator tone=syx.begin()+3712;
		for (int i=0; i<50; ++i)
		{
			SyxBuffer::const_iterator tone1=tone+i*69;
			std::string toneName1(tone1+9,tone1+19);
			FixNameNumbers(toneName1);
			std::cerr << std::setfill('0') << std::setw(2) << i+1 << " " << toneName1;
			if (printPresets)
				std::cerr << "              " << std::setfill(' ') << std::setw(3) << i+51 << " " << presetTones[i];
			std::cerr << std::endl;
		}
	}
	
	static void PrintUsedTones(const ToneNumberSet& tones, bool includePresets)
	{
		std::cerr << "User tones in use: ";
		for (ToneNumberSet::const_iterator it=tones.begin(); it!=tones.end(); ++it)
		{
			int tone = *it+1;
			if (tone<=50)
				std::cerr << tone << " ";
		}
		if (includePresets)
		{
			std::cerr << std::endl << "Factory preset tones in use: ";
			for (ToneNumberSet::const_iterator it=tones.begin(); it!=tones.end(); ++it)
			{
				int tone = *it+1;
				if (tone>50)
					std::cerr << tone << " ";
			}
		}
		std::cerr << std::endl;
	}
	
	static void PrintUnusedTones(const ToneNumberSet& tones, bool includePresets)
	{
		std::cerr << "User tones not in use: ";
		ToneNumberSet allTones;
		for (int i=0; i<100; ++i) allTones.insert(i);
		for (ToneNumberSet::const_iterator it=tones.begin(); it!=tones.end(); ++it)
		{
			allTones.erase(*it);
		}
		for (ToneNumberSet::const_iterator it=allTones.begin(); it!=allTones.end(); ++it)
		{
			int tone = *it+1;
			if (tone<=50)
				std::cerr << tone << " ";
		}
		if (includePresets)
		{
			std::cerr << std::endl << "Factory preset tones not in use: ";
			for (ToneNumberSet::const_iterator it=allTones.begin(); it!=allTones.end(); ++it)
			{
				int tone = *it+1;
				if (tone>50)
					std::cerr << tone << " ";
			}
		}
		std::cerr << std::endl;
	}
	
	static void PrintHelp(const char * appName)
	{
        std::cerr << "\
usage: superjxsyxtool [-h] [-p] [-u] [-U] [-f] [-x patch1,patch2,tone1,tone2,...]\n\
                      [-s COPYFROMFILE [-c src1,dst1,src2,dst2,...]] [-o OUTPUTFILE] INPUTFILE\n\n\
Manipulates Roland Super JX (MKS-70 / JX-10) system exclusive (syx) files.\n\n\
positional arguments:\n\
  INPUTFILE                           the input syx file\n\n\
optional arguments:\n\
  -h                                  show this help message and exit\n\
  -o OUTPUTFILE                       write changed syx to OUTPUTFILE\n\
  -p                                  print all patches and tones\n\
  -u                                  print tones used by patches\n\
  -U                                  print unused tones\n\
  -v                                  include factory preset tones in printed output\n\
  -x patch1,patch2,tone1,tone2,...    exchange patch1 from INPUTFILE with patch2, tone1 with\n\
                                      tone2, and so on (requires pairs of patch/tone addresses),\n\
                                      updating patches that use exchanged tones\n\
  -X patch1,patch2,tone1,tone2,...    exchange patch1 from INPUTFILE with patch2, tone1 with\n\
                                      tone2, and so on (requires pairs of patch/tone addresses),\n\
                                      *without* updating patches that use exchanged tones\n\
  -s COPYFROMFILE                     use COPYFROMFILE syx as the source for a -c operation\n\
  -c src1,dst1,src2,dst2,...          copy src1 from COPYFROMFILE to dst1, src2 to dst1,\n\
                                      and so on (requires pairs of patch/tone addresses)\n\
  -f                                  force copying patches even if destination user tones are\n\
                                      in use by another patch\n\
        " << std::endl;
	}
	
} // anonymous namespace

int main(int argc, const char * argv[])
{
	if (argc == 1)
	{
		PrintHelp(argv[0]);
	}
	else
	{
		SyxBuffer syx = ReadFile(argv[argc-1]);
		
		std::string outputFile;
		
		if (!syx.empty())
		{
			Patches patches = ParsePatches(syx);
			Tones tones = ParseTones(syx);
			ToneNumberSet usedTones = ParseUsedTones(patches);

			Swaps swaps;
            Swaps copies;
            std::string sourceFile;
			bool print = false;
			bool printUsed = false;
			bool printUnused = false;
			bool includePresets = false;
			
			for (size_t a=1; a<argc-1; ++a)
			{
				if (strcmp(argv[a],"-h")==0) // help
				{
					PrintHelp(argv[0]);
					return 0;
				}
                else if (strcmp(argv[a],"-c")==0 && a+2<argc) // copy pairs from source to dest
                {
                    copies = ParseSwaps(argv[a+1]);
                    ++a;
                }
                else if (strcmp(argv[a],"-s")==0 && a+2<argc) // source file for copying patches and tones
                {
                    sourceFile = argv[a+1];
                    ++a;
                }
				else if (strcmp(argv[a],"-o")==0 && a+2<argc) // write to output file
				{
					outputFile = argv[a+1];
					++a;
				}
				else if (strcmp(argv[a],"-p")==0) // list patches & tones
				{
					print = true;
				}
				else if (strcmp(argv[a],"-v")==0) // include factory presets
				{
					includePresets = true;
				}
				else if (strcmp(argv[a],"-u")==0) // list used tones
				{
					printUsed = true;
				}
				else if (strcmp(argv[a],"-U")==0) // list unused tones
				{
					printUnused = true;
				}
				else if (strcmp(argv[a],"-x")==0 && a+2<argc) // exchange pairs of patches & tones
				{
					swaps = ParseSwaps(argv[a+1]);
					++a;
				}
			}
			
			SwapPatches(swaps.first,patches);
            SwapTones(swaps.second,patches,tones);

            CopyPatchesAndTones(copies,patches,tones);
            
			syx = WriteSyx(patches,tones);

			if (print)
			{
				SyxBuffer syxForPrint(syx);
				CompressPatches(syxForPrint);
				PrintPatchesAndTones(syxForPrint,includePresets);
			}

			if (printUsed)
				PrintUsedTones(usedTones,includePresets);
			if (printUnused)
				PrintUnusedTones(usedTones,includePresets);

			if (!outputFile.empty())
				WriteFile(outputFile.c_str(),syx);
		}
        else
        {
            PrintHelp(argv[0]);
        }
	}
	
	return 0;
}


