//
//  main.cpp
//  superjxsyxtool
//
//  Created by Christian E. on 2017/March/09
//  Copyright © 2017 Ten by Ten. All rights reserved.
//

#include "sysex.h"
#include "tone.h"
#include "patch.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <vector>

namespace
{
    static const size_t kOffset_MIDIChannel = 3;
    static const size_t kOffset_Address = 8;
	
    typedef std::vector<SyxBuffer> SyxPatches;
    typedef std::vector<SyxBuffer> SyxTones;
    typedef std::set<uint8_t> PatchNumberSet;
    typedef std::set<uint8_t> ToneNumberSet;
    typedef std::pair<uint8_t, uint8_t> AddressPair;
    typedef std::vector<AddressPair> PatchSwaps;
    typedef std::vector<AddressPair> ToneSwaps;
    typedef std::pair<PatchSwaps, ToneSwaps> Swaps;
    
    static const char* presetTones[] = {
        "PIANO 1    ", "E. GRAND 1 ", "PIANO 2    ", "CELLO SECT ", "ARCO STRING",
        "LOW STRINGS", "HI STRINGS ", "BEE-THREE  ", "ORGAN 1    ", "CALIOPE    ",
        "PIPE ORGAN ", "DRYSTLDRUM ", "MUSIC BOX  ", "WINDCHIMES ", "E. BASS    ",
        "SYNTH BASS ", "SOUNDTRACK ", "HOLLOW PAD ", "FLUTE 1    ", "FRETNOT 1  ",
        "BIG OL PAD ", "STABBRASS 2", "POLYSYNTH 2", "GOWESTBRS 2", "GOWESTBRS 1",
        "POLY BRASS ", "GAMELANET  ", "CELESTE 2  ", "AGOGO BELL ", "SYNDULCIMR ",
        "GUITARCLAV ", "PERKPIANO  ", "PIANO 4    ", "SYNC LEAD  ", "SEQ* 1     ",
        "RECORDERS  ", "BRIGHT BOWS", "STRINGS 1  ", "STRINGS 2  ", "CHOIR      ",
        "MAY,S WIND ", "MARIMBA    ", "METALLET   ", "SYNTHBELL 2", "XMAS BELLS ",
        "VIBES      ", "CHURCHBELL ", "RES BELL   ", "KALIMBA 2  ", "GOWESTVOX  ", };
    
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
    
//    static void DumpSyxBuffer(const SyxBuffer& buffer)
//    {
//        std::cerr << std::endl;
//        for (SyxBuffer::const_iterator it=buffer.begin(); it!=buffer.end(); ++it)
//        {
//            std::cerr << std::hex << std::setfill('0') << std::setw(2) << (int)*it << " ";
//        }
//        std::cerr << std::endl;
//    }
	
    static void WriteFile(const char* path, const SyxBuffer& syx)
    {
        std::ofstream file(path, std::ios::out | std::ios::binary);
        if (file.good())
            file.write((const char*)&syx.at(0),syx.size());
    }
    
    static bool FindPatch(const SyxBuffer& syx, SyxBuffer::const_iterator& from)
    {
        SyxBuffer::const_iterator patchStart = from;
        do
        {
            // patchStart[2]
            // 0x34 = PGR    0x35 = APR    0x36 = IPR    0x37 = BLD
            if ((patchStart < syx.end())
                && (patchStart+105 < syx.end())
                && (patchStart[0] == 0xf0)
                && (patchStart[1] == 0x41) // Roland
                && (patchStart[4] == 0x24) // MKS-70 / JX-10
                && (patchStart[5] == 0x30) // Patch Data
				&& (patchStart[SuperJXPatch::kPatchSize-1] == 0xf7)) // end of patch data (106 bytes in file)
            {
                from = patchStart;
                return true;
            }
        }
        while (++patchStart < syx.end());
        return false;
    }
    
    static size_t FindTone(const SyxBuffer& syx, SyxBuffer::const_iterator& from)
    {
        SyxBuffer::const_iterator it = from;
        do
        {
            // toneStart[2]
            // 0x34 = PGR    0x35 = APR    0x36 = IPR    0x37 = BLD
            if ((it < syx.end())
                && (it[0] == 0xf0)
                && (it[1] == 0x41) // Roland
                && (it[4] == 0x24) // MKS-70 / JX-10
                && (it[5] == 0x20) // Tone Data
				&& ((it[SuperJXTone::kToneSize-1] == 0xf7) || (it[SuperJXTone::kToneSize_Short-1] == 0xf7))) // end of tone data
            {
                from = it;
                while ((*(++it) != 0xf7) && (it < syx.end())) ;
                if (it < syx.end())
                {
                    return it-from+1;
                }
            }
        }
        while (++it < syx.end());
        return 0;
    }
    
    static SyxPatches ParsePatches(const SyxBuffer& syx)
    {
        SyxPatches patches;
        SyxBuffer::const_iterator it = syx.begin();
        for (int b=0; b<64; ++b)
        {
            if (FindPatch(syx,it))
            {
                SyxBuffer patch(it,it+SuperJXPatch::kPatchSize);
				it+=SuperJXPatch::kPatchSize;
                patches.push_back(patch);
            }
            else
                return SyxPatches();
        }
        return patches;
    }
    
    static SyxTones ParseTones(const SyxBuffer& syx)
    {
        SyxTones tones;
        size_t toneSize = 0;
        SyxBuffer::const_iterator it = syx.begin();
        do
        {
            toneSize = FindTone(syx,it);
            if (toneSize>0)
            {
                SyxBuffer tone(it,it+toneSize);
                it+=toneSize;
                tones.push_back(tone);
            }
        } while (toneSize!=0 && it<syx.end());
        return tones;
    }
        
    static ToneNumberSet ParseUsedTones(const SyxPatches& banks)
    {
        ToneNumberSet tones;
		for (auto& patch: banks)
        {
            uint8_t a,b;
            bool holda,holdb;
			SuperJXPatch::GetTones(patch,a,b,holda,holdb);
            tones.insert(a);
            tones.insert(b);
        }
        return tones;
    }
    
    static bool IsPatch(const std::string& str)
    {
        return ((str.size()==2)
                && ((str[0] >= 'A' && str[0] <='H')
                    || (str[0] >= 'a' && str[0] <= 'h'))
                && (str[1] >= '1' && str[1] <= '8'));
    }
    
    static uint8_t ParsePatchNumber(const std::string& str)
    {
        if (IsPatch(str))
        {
            uint8_t b = str[0] > 'H' ? str[0]-'a' : str[0]-'A';
            uint8_t p = str[1] - '1';
            return b*8+p;
        }
        return 0;
    }
    
    static uint8_t ParseToneNumber(const std::string& str)
    {
        return static_cast<uint8_t>(strtoul(str.c_str(),NULL,10)-1);
    }
    
    static bool IsUserTone(const std::string& str)
    {
        uint8_t t = ParseToneNumber(str);
        return t>=0 && t<51;
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
                    AddressPair AddressPair(ParsePatchNumber(n1),ParsePatchNumber(n2));
                    swaps.first.push_back(AddressPair);
                }
                else if (IsUserTone(n1) && IsUserTone(n2))
                {
                    AddressPair AddressPair(ParseToneNumber(n1),ParseToneNumber(n2));
                    swaps.second.push_back(AddressPair);
                }
                else
                    break;
            }
            else
                break;
        }
        return swaps;
    }
    
    static void SwapPatchTones(SyxPatches& patches, uint8_t t1, uint8_t t2)
    {
		for (auto& patch: patches)
        {
            uint8_t a,b;
            bool holda,holdb;
			SuperJXPatch::GetTones(patch,a,b,holda,holdb);
            if (a == t1)
                a = t2;
            else if (a == t2)
                a = t1;
            if (b == t1)
                b = t2;
            else if (b == t2)
                b = t1;
			SuperJXPatch::SetTones(patch,a,b,holda,holdb);
        }
    }
    
    static void SwapPatches(const PatchSwaps& swaps, SyxPatches& patches)
    {
        if (!swaps.empty())
        {
            std::cerr << "Swapping patches:";
            for (PatchSwaps::const_iterator it=swaps.begin(); it!=swaps.end(); ++it)
            {
                uint8_t p1 = it->first;
                uint8_t p2 = it->second;
                std::cerr << " " << SuperJXPatch::GetPatchAddress(p1) << "<=>" << SuperJXPatch::GetPatchAddress(p2);
                SyxPatches::iterator it1 = patches.begin()+p1;
                SyxPatches::iterator it2 = patches.begin()+p2;
                std::swap(*it1,*it2);
            }
            std::cerr << std::endl;
        }
    }
    
    static void SwapTones(const ToneSwaps& swaps, SyxPatches& patches, SyxTones& tones)
    {
        if (!swaps.empty())
        {
            std::cerr << "Swapping tones:";
            for (ToneSwaps::const_iterator it=swaps.begin(); it!=swaps.end(); ++it)
            {
                // when swapping tones, make sure the patches are updated too
                uint8_t t1 = it->first;
                uint8_t t2 = it->second;
                SwapPatchTones(patches,t1,t2);
                std::cerr << " " << (int)t1+1 << "<=>" << (int)(t2+1);
                SyxPatches::iterator it1 = tones.begin()+t1;
                SyxPatches::iterator it2 = tones.begin()+t2;
                std::swap(*it1,*it2);
            }
            std::cerr << std::endl;
        }
    }
    
    static ToneNumberSet InvertToneNumberSet(const ToneNumberSet& tones)
    {
        ToneNumberSet itones;
        for (int i=0; i<100; ++i) itones.insert(i);
        for (ToneNumberSet::const_iterator it=tones.begin(); it!=tones.end(); ++it)
            itones.erase(*it);
        return itones;
    }
    
    std::ostream & operator<<(std::ostream &os, const ToneNumberSet& tones)
    {
		for (auto t: tones)
            os << (int)t+1 << " ";
        return os;
    }

    static bool FindMatchingTone(const SyxBuffer& tone, const SyxTones& otherTones, uint8_t& matchingOtherToneNumber)
    {
        uint8_t t = 0;
        for (SyxTones::const_iterator it=otherTones.begin(); it!=otherTones.end(); ++it, ++t)
        {
            if ((memcmp(&tone.at(0),&it->at(0),kOffset_Address)==0)
				&& (memcmp(&tone.at(0)+kOffset_Address+1,&it->at(0)+kOffset_Address+1,tone.size()-kOffset_Address-1)==0))
            {
				matchingOtherToneNumber = t;
                return true;
            }
        }
        return false;
    }
    
    static void CopyPatchesAndTones(const Swaps& swaps, const bool overwriteUnused, const bool force,
                                    const SyxPatches& srcPatches, const SyxTones& srcTones,
                                    SyxPatches& dstPatches, SyxTones& dstTones)
    {
        const PatchSwaps& patchSwaps = swaps.first;
        ToneSwaps toneSwaps = swaps.second; // this is a copy; it will be changed below
        
        if (!patchSwaps.empty() || !toneSwaps.empty())
        {
            // first identify which tones can be overwritten by counting tones used by current uncopied patches
            ToneNumberSet unusedTones = InvertToneNumberSet(ToneNumberSet());
            if (overwriteUnused)
            {
                SyxPatches leftoverPatches;
                PatchNumberSet leftoverPatchNumbers;
                for (int i=0; i<64; ++i) leftoverPatchNumbers.insert(i);
                for (PatchSwaps::const_iterator it = patchSwaps.begin(); it!=patchSwaps.end(); ++it)
                    leftoverPatchNumbers.erase(it->second);
                for (PatchNumberSet::iterator it=leftoverPatchNumbers.begin(); it!=leftoverPatchNumbers.end(); ++it)
                    leftoverPatches.push_back(*(dstPatches.begin()+*it));
                ToneNumberSet usedTones = ParseUsedTones(leftoverPatches);
                unusedTones = InvertToneNumberSet(usedTones);
            }
            for (int i=50; i<100; ++i)
                unusedTones.erase(i); // factory tones are not candidates to overwrite
            if (overwriteUnused)
            {
                std::cerr << "Unused tones that can be overwritten: " << unusedTones << std::endl;
            }
            
            // next, identify user tones that must be copied that are not yet specified in swaps
            ToneNumberSet implicitlyRequiredTones;
            for (PatchSwaps::const_iterator it = patchSwaps.begin(); it!=patchSwaps.end(); ++it)
            {
                const SyxBuffer& patch=srcPatches[it->first];
                uint8_t a,b;
                bool holda,holdb;
                SuperJXPatch::GetTones(patch,a,b,holda,holdb);
                // copy is required only if the tones are not already in dstTones
				// -- but then the source patch's tones must be renumbered, so treat this as a swap
				if (a < 50)
				{
					uint8_t dstTone;
					if (FindMatchingTone(srcTones[a], dstTones, dstTone))
					{
						toneSwaps.push_back(AddressPair(a, dstTone));
					}
					else
						implicitlyRequiredTones.insert(a);
				}
				if (b < 50)
				{
					uint8_t dstTone;
					if (FindMatchingTone(srcTones[b], dstTones, dstTone))
					{
						toneSwaps.push_back(AddressPair(b, dstTone));
					}
					else
						implicitlyRequiredTones.insert(b);
				}
            }
            std::cerr << "Tones required from source patches: " << implicitlyRequiredTones << std::endl;

            // tones specified in toneSwaps will be copied directly; no need to overwrite unused tones with those
            for (ToneSwaps::const_iterator it = toneSwaps.begin(); it!=toneSwaps.end(); ++it)
                implicitlyRequiredTones.erase(it->first);
            std::cerr << "Tones to overwrite unused tones: " << implicitlyRequiredTones << std::endl;
            
            if (!force)
            {
                // validate by looking for tones used by src & dest patches
                // then look for other patches using these tones
                if (unusedTones.size() < implicitlyRequiredTones.size())
                {
                    std::cerr << "Not enough unused tones available to copy patches. Need: "
                        << implicitlyRequiredTones.size() << " have: " << unusedTones.size() << std::endl;
                    return;
                }
            }
            
            // now upgrade toneSwaps to include required tones and map to unused ones
            ToneNumberSet::iterator it=implicitlyRequiredTones.begin();
            ToneNumberSet::iterator uit=unusedTones.begin();
            for (; it!=implicitlyRequiredTones.end() && uit!=unusedTones.end(); ++it, ++uit)
            {
                toneSwaps.push_back(AddressPair(*it,*uit));
            }
            // there should only be more implicitlyRequiredTones than unusedTones if force is true
            for (; it!=implicitlyRequiredTones.end(); ++it)
            {
                toneSwaps.push_back(AddressPair(*it,*it));
            }

            if (!patchSwaps.empty())
            {
                std::cerr << "Copying patches:";
                for (PatchSwaps::const_iterator it=patchSwaps.begin(); it!=patchSwaps.end(); ++it)
                {
                    uint8_t p1 = it->first;
                    uint8_t p2 = it->second;
                    std::cerr << " " << SuperJXPatch::GetPatchAddress(p1) << "->" << SuperJXPatch::GetPatchAddress(p2);
                    SyxPatches::const_iterator it1 = srcPatches.begin()+p1;
                    SyxPatches::iterator it2 = dstPatches.begin()+p2;
                    *it2 = *it1;
                    // now fix up tones
                    uint8_t a,b;
                    bool holda,holdb;
                    SuperJXPatch::GetTones(*it2,a,b,holda,holdb);
                    if (a<50)
                    {
                        for (ToneSwaps::const_iterator it=toneSwaps.begin(); it!=toneSwaps.end(); ++it)
                            if (a == it->first)
                            {
                                a = it->second;
                                break;
                            }
                    }
                    if (b<50)
                    {
                        for (ToneSwaps::const_iterator it=toneSwaps.begin(); it!=toneSwaps.end(); ++it)
                            if (b == it->first)
                            {
                                b = it->second;
                                break;
                            }
                    }
                    SuperJXPatch::SetTones(*it2,a,b,holda,holdb);
                }
                std::cerr << std::endl;
            }
            if (!toneSwaps.empty())
            {
                std::cerr << "Copying tones:";
                for (ToneSwaps::const_iterator it=toneSwaps.begin(); it!=toneSwaps.end(); ++it)
                {
                    uint8_t t1 = it->first;
                    uint8_t t2 = it->second;
                    if (t1<50 && t2<50)
                    {
                        std::cerr << " " << (int)t1+1 << "->" << (int)(t2+1);
                        SyxTones::const_iterator it1 = srcTones.begin()+t1;
                        SyxTones::iterator it2 = dstTones.begin()+t2;
                        *it2 = *it1;
                    }
                }
                std::cerr << std::endl;
            }
        }
    }
    
    static SyxBuffer WriteSyx(SyxPatches& patches, SyxTones& tones)
    {
        uint8_t p=0, t=0;
        SyxBuffer syx;
        for (SyxPatches::iterator it=patches.begin(); it!=patches.end(); ++it)
        {
            *(it->begin()+kOffset_MIDIChannel) = (uint8_t)0; // set midi channel to 1
            *(it->begin()+kOffset_Address) = p++; // important! renumber the output patches
            syx.insert(syx.end(),it->begin(),it->end());
        }
        for (SyxTones::iterator it=tones.begin(); it!=tones.end(); ++it)
        {
            SyxBuffer tone(it->begin(),it->end());
			if (tone.size() == SuperJXTone::kToneSize_Short)
            {
                // convert PGR, APR, IPR tones to BLD
                tone.insert(tone.begin()+7,0);
                tone.insert(tone.begin()+7,0);
            }
            tone[2] = 0x37; // BLD
            tone[kOffset_MIDIChannel] = 0; // set midi channel to 1
            tone[kOffset_Address] = t++; // important! renumber the output patches
            syx.insert(syx.end(),tone.begin(),tone.end());
        }
        return syx;
    }
    
    static void PrintPatchName(const SyxBuffer& patch, uint8_t p)
    {
        std::cerr << " " << SuperJXPatch::GetPatchAddress(p) << "  ";
		std::string patchName = SuperJXPatch::GetName(patch);
        uint8_t a,b;
        bool holda,holdb;
        SuperJXPatch::GetTones(patch,a,b,holda,holdb);
        std::cerr << patchName << "    " << std::setfill(' ') << std::setw(3) << (int)b+1 << "  " << std::setw(3) << (int)a+1;
    }

    static void PrintToneName(const SyxBuffer& tone, uint8_t t)
    {
		std::string toneName = SuperJXTone::GetName(tone);
        std::cerr << std::setfill('0') << std::setw(2) << t+1 << "  " << toneName;
    }
    
    static void PrintPatchesAndTones(const SyxPatches& patches, const SyxTones& tones, const bool printPresets)
    {
        std::cerr << "-----------------------------------" << std::endl;
        std::cerr << "PATCH      NAME              B    A" << std::endl;
        std::cerr << "-----------------------------------" << std::endl;
        for (int p=0; p<patches.size()/2; ++p)
        {
            PrintPatchName(patches[p],p);
            std::cerr << "          ";
            PrintPatchName(patches[p+patches.size()/2],p+patches.size()/2);
            std::cerr << std::endl;
            if ((p+1)%8==0) std::cerr << std::endl;
        }
        std::cerr << "--------------" << std::endl;
        std::cerr << "T#   TONE NAME" << std::endl;
        std::cerr << "--------------" << std::endl;
        size_t rowCount = tones.size() == 1 ? 1 : tones.size()/5;
        for (int t=0; t<rowCount; ++t)
        {
            PrintToneName(tones[t],t);
            if (rowCount < tones.size())
            {
                std::cerr << "     ";
                PrintToneName(tones[t+rowCount],t+rowCount);
                std::cerr << "     ";
                PrintToneName(tones[t+rowCount*2],t+rowCount*2);
                std::cerr << "     ";
                PrintToneName(tones[t+rowCount*3],t+rowCount*3);
                std::cerr << "     ";
                PrintToneName(tones[t+rowCount*4],t+rowCount*4);
                std::cerr << std::endl;
            }
        }
        if (printPresets)
        {
            std::cerr << std::endl;
            for (int t=0; t<50; ++t)
            {
                std::cerr << (int)t+51 << "  " << presetTones[t];
                if ((t+1)%5)
                    std::cerr << "    ";
                else
                    std::cerr << std::endl;
            }
        }
    }
    
    static void PrintUsedTones(const ToneNumberSet& tones, const bool includePresets)
    {
        std::cerr << "User tones in use: ";
		for (auto t: tones)
        {
            int tone = t+1;
            if (tone<=50)
                std::cerr << tone << " ";
        }
        if (includePresets)
        {
            std::cerr << std::endl << "Factory preset tones in use: ";
			for (auto t: tones)
            {
                int tone = t+1;
                if (tone>50)
                    std::cerr << tone << " ";
            }
        }
        std::cerr << std::endl;
    }
    
    static void PrintUnusedTones(const ToneNumberSet& usedTones, const bool includePresets)
    {
        std::cerr << "User tones not in use: ";
        ToneNumberSet unusedTones = InvertToneNumberSet(usedTones);
		for (auto t: unusedTones)
        {
            int tone = t+1;
            if (tone<=50)
                std::cerr << tone << " ";
        }
        if (includePresets)
        {
            std::cerr << std::endl << "Factory preset tones not in use: ";
			for (auto t: unusedTones)
            {
                int tone = t+1;
                if (tone>50)
                    std::cerr << tone << " ";
            }
        }
        std::cerr << std::endl;
    }
	
	static void ParseDetails(const char* str, PatchNumberSet& patches, ToneNumberSet& tones)
	{
		std::stringstream ss;
		ss.str(str);
		std::string n1;
		while (std::getline(ss, n1, ','))
		{
			if (IsPatch(n1))
			{
				patches.insert(ParsePatchNumber(n1));
			}
			else if (IsUserTone(n1))
			{
				tones.insert(ParseToneNumber(n1));
			}
		}
	}
	
	static void PrintDetails(const SyxPatches& patches, const SyxTones& tones, const PatchNumberSet& whichPatches, const ToneNumberSet& whichTones)
	{
		bool printAll = whichPatches.empty() && whichTones.empty();
		std::cerr << std::endl << "----------------" << std::endl;
		std::cerr << "PARAMETERS" << std::endl;
		
		if (printAll)
		{
			for (auto& p: patches)
			{
				SuperJXPatch patch(p);
				patch.Print();
			}
			for (auto& t: tones)
			{
				SuperJXTone tone(t);
				tone.Print();
			}
		}
		else
		{
			for (auto p: whichPatches)
			{
				SuperJXPatch patch(patches[p]);
				patch.Print();
			}
			for (auto t: whichTones)
			{
				SuperJXTone tone(tones[t]);
				tone.Print();
			}
		}
	}
	
	static void PrintHelp(const char * appName)
	{
        std::cerr << "\
usage: superjxsyxtool [-h] [-p] [-u] [-U] [-f]\n\
                      [-d [patch1,patch2,tone1,tone2,...]]\n\
                      [-x patch1,patch2,tone1,tone2,...]\n\
                      [-s COPYFROMFILE [-c src1,dst1,src2,dst2,...]] [-o OUTPUTFILE] INPUTFILE\n\n\
Manipulates Roland Super JX (MKS-70 / JX-10) system exclusive (syx) files.\n\n\
positional arguments:\n\
  INPUTFILE                           the input syx file\n\n\
optional arguments (addresses for patches: a1-h8 or A1-H8; user tones: 1-50):\n\
  -h                                  show this help message and exit\n\
  -d [patch1,tone1,...]               print parameter details of the specified patches and tones,\n\
                                      or details of all patches and tones of none are specified\n\
  -o OUTPUTFILE                       write changed syx to OUTPUTFILE\n\
  -p                                  print all patches and tones\n\
  -u                                  print tones used by patches\n\
  -U                                  print unused tones\n\
  -v                                  include factory preset tones in printed output\n\
  -x patch1,patch2,tone1,tone2,...    AddressPair patch1 from INPUTFILE with patch2, tone1 with\n\
                                      tone2, and so on (requires pairs of patch/tone addresses),\n\
                                      updating patches that use AddressPaird tones\n\
  -X patch1,patch2,tone1,tone2,...    AddressPair patch1 from INPUTFILE with patch2, tone1 with\n\
                                      tone2, and so on (requires pairs of patch/tone addresses),\n\
                                      *without* updating patches that use AddressPaird tones\n\
  -s COPYFROMFILE                     use COPYFROMFILE syx as the source for a -c operation\n\
  -c src1,dst1,src2,dst2,...          copy src1 from COPYFROMFILE to dst1, src2 to dst1,\n\
                                      and so on (requires pairs of patch/tone addresses); \n\
                                      new user tones are automatically copied for each patch,\n\
                                      overwriting unused tones as needed\n\
  -f                                  force copying tones that are in use by another patch,\n\
                                      if there are insufficient unused tones to overwrite\n\
  -F                                  force copying tones that are in use by another patch,\n\
                                      and do not overwrite any unused tones\n" << std::endl;
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
            SyxPatches patches = ParsePatches(syx);
            SyxTones tones = ParseTones(syx);
            
            Swaps swaps;

            Swaps copies;
            SyxPatches srcPatches;
            SyxTones srcTones;

            bool print = false;
            bool printUsed = false;
            bool printUnused = false;
            bool includePresets = false;
            bool forceAddressPair = false;
            bool forceCopy = false;
            bool overwriteUnused = true;

			PatchNumberSet patchDetails;
			ToneNumberSet toneDetails;
			bool printDetails = false;
            
            for (size_t a=1; a<argc-1; ++a)
            {
                if (strcmp(argv[a],"-c")==0 && a+2<argc) // copy pairs from source to dest
                {
                    copies = ParseSwaps(argv[a+1]);
                    ++a;
                }
                else if (strcmp(argv[a],"-F")==0) // force copy don't overwrite unused tones
                {
                    forceCopy = true;
                    overwriteUnused = false;
                }
                else if (strcmp(argv[a],"-f")==0) // force copy
                {
                    forceCopy = true;
                }
                else if (strcmp(argv[a],"-h")==0) // help
                {
                    PrintHelp(argv[0]);
                    return 0;
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
                else if (strcmp(argv[a],"-s")==0 && a+2<argc) // source file for copying patches and tones
                {
                    SyxBuffer srcSyx = ReadFile(argv[a+1]);
                    ++a;
                    if (!srcSyx.empty())
                    {
                        srcPatches = ParsePatches(srcSyx);
                        srcTones = ParseTones(srcSyx);
                    }
                }
                else if (strcmp(argv[a],"-u")==0) // list used tones
                {
                    printUsed = true;
                }
                else if (strcmp(argv[a],"-U")==0) // list unused tones
                {
                    printUnused = true;
                }
                else if (strcmp(argv[a],"-v")==0) // include factory presets
                {
                    includePresets = true;
                }
                else if ((strcmp(argv[a],"-x")==0 || strcmp(argv[a],"-X")==0) && a+2<argc) // AddressPair pairs of patches & tones
                {
                    swaps = ParseSwaps(argv[a+1]);
                    forceAddressPair = (strcmp(argv[a],"-X")==0);
                    ++a;
                }
				else if (strcmp(argv[a],"-d")==0)
				{
					ParseDetails(argv[a+1],patchDetails,toneDetails);
					if (!patchDetails.empty() || !toneDetails.empty())
						++a;
					printDetails = true;
				}
            }
            
            SwapPatches(swaps.first,patches);

            SyxPatches noPatches;
            SwapTones(swaps.second,forceAddressPair?noPatches:patches,tones);
            
            CopyPatchesAndTones(copies,overwriteUnused,forceCopy,srcPatches,srcTones,patches,tones);
            
            syx = WriteSyx(patches,tones);
            
            // if there's nothing else to do then print
            if (swaps.first.empty() && swaps.second.empty() && copies.first.empty() && copies.second.empty() && patchDetails.empty() && toneDetails.empty())
                print = true;
            
			patches = ParsePatches(syx);
			tones = ParseTones(syx);

			if (print)
            {
                PrintPatchesAndTones(patches,tones,includePresets);
            }

            ToneNumberSet usedTones = ParseUsedTones(patches);
            if (printUsed)
                PrintUsedTones(usedTones,includePresets);
            if (printUnused)
                PrintUnusedTones(usedTones,includePresets);
            
            if (!outputFile.empty())
                WriteFile(outputFile.c_str(),syx);
			
			if (printDetails)
				PrintDetails(patches,tones,patchDetails,toneDetails);
		}
        else
        {
            PrintHelp(argv[0]);
        }
    }
    
    return 0;
}


