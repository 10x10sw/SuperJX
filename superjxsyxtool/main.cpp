//
//  main.cpp
//  superjxsyxtool
//
//  Created by Christian E. on 2017/March/09
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
    typedef std::set<unsigned char> PatchNumberSet;
    typedef std::set<unsigned char> ToneNumberSet;
    typedef std::pair<unsigned char, unsigned char> AddressPair;
    typedef std::vector<AddressPair> PatchSwaps;
    typedef std::vector<AddressPair> ToneSwaps;
    typedef std::pair<PatchSwaps, ToneSwaps> Swaps;
    
    const char* presetTones[] = {
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
                && (patchStart[105] == 0xf7)) // end of patch data (106 bytes in file)
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
                && ((it[68] == 0xf7) || (it[66] == 0xf7))) // end of tone data
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
        } while (toneSize != 0);
        return tones;
    }
    
    std::string GetPatchName(const SyxBuffer& patch)
    {
        std::string name;
        for (int i=0; i<18; i++)
        {
            unsigned char c = CombineBytes(patch[9+i*2], patch[10+i*2]);
            if (c < 10)
                c += 48;
            name.push_back(c);
        }
        return name;
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
            unsigned char a,b;
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
    
    bool IsUserTone(const std::string& str)
    {
        unsigned char t = ParseToneNumber(str);
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
    
    static void SwapPatchTones(Patches& patches, unsigned char t1, unsigned char t2)
    {
        for (Patches::iterator it=patches.begin(); it!=patches.end(); ++it)
        {
            unsigned char a,b;
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
    
//    PatchNumberSet InvertPatchNumberSet(const PatchNumberSet& patches)
//    {
//        PatchNumberSet ipatches;
//        for (int i=0; i<patches.size(); ++i) ipatches.insert(i);
//        for (PatchNumberSet::const_iterator it=patches.begin(); it!=patches.end(); ++it) ipatches.erase(*it);
//        return ipatches;
//    }
    
    ToneNumberSet InvertToneNumberSet(const ToneNumberSet& tones)
    {
        ToneNumberSet itones;
        for (int i=0; i<100; ++i) itones.insert(i);
        for (ToneNumberSet::const_iterator it=tones.begin(); it!=tones.end(); ++it)
            itones.erase(*it);
        return itones;
    }
    
    std::ostream & operator<<(std::ostream &os, const ToneNumberSet& tones)
    {
        for (ToneNumberSet::const_iterator it=tones.begin(); it!=tones.end(); ++it)
            os << (int)(*it)+1 << " ";
        return os;
    }

    static int Compare(const Tones& srcTones, Tones& dstTones, unsigned char tone)
    {
        return memcmp(&srcTones[tone].at(0),&dstTones[tone].at(0),srcTones[tone].size());
    }
    
    static void CopyPatchesAndTones(const Swaps& swaps, const bool overwriteUnused, const bool force,
                                    const Patches& srcPatches, const Tones& srcTones,
                                    Patches& dstPatches, Tones& dstTones)
    {
        const PatchSwaps& patchSwaps = swaps.first;
        ToneSwaps toneSwaps = swaps.second; // this is a copy; it will be changed below
        
        if (!patchSwaps.empty() || !toneSwaps.empty())
        {
            // first identify which tones can be overwritten by counting tones used by current uncopied patches
            ToneNumberSet unusedTones = InvertToneNumberSet(ToneNumberSet());
            if (overwriteUnused)
            {
                Patches leftoverPatches;
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
            
            // identify user tones that must be copied that are not yet specified in swaps
            ToneNumberSet implicitlyRequiredTones;
            for (PatchSwaps::const_iterator it = patchSwaps.begin(); it!=patchSwaps.end(); ++it)
            {
                const SyxBuffer& patch=srcPatches[it->first];
                unsigned char a,b;
                GetPatchTones(patch,a,b);
                // copy is required only if the tones *actually* differ
                if (a<50 && Compare(srcTones,dstTones,a)!=0)
                    implicitlyRequiredTones.insert(a);
                if (b<50 && Compare(srcTones,dstTones,b)!=0)
                    implicitlyRequiredTones.insert(b);
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
                    unsigned char p1 = it->first;
                    unsigned char p2 = it->second;
                    std::cerr << " " << (char)(65+p1/8) << p1%8+1 << "->" << (char)(65+p2/8) << p2%8+1;
                    Patches::const_iterator it1 = srcPatches.begin()+p1;
                    Patches::iterator it2 = dstPatches.begin()+p2;
                    *it2 = *it1;
                    // now fix up tones
                    unsigned char a,b;
                    GetPatchTones(*it2,a,b);
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
                    SetPatchTones(*it2,a,b);
                }
                std::cerr << std::endl;
            }
            if (!toneSwaps.empty())
            {
                std::cerr << "Copying tones:";
                for (ToneSwaps::const_iterator it=toneSwaps.begin(); it!=toneSwaps.end(); ++it)
                {
                    unsigned char t1 = it->first;
                    unsigned char t2 = it->second;
                    if (t1<50 && t2<50)
                    {
                        std::cerr << " " << (int)t1+1 << "->" << (int)(t2+1);
                        Tones::const_iterator it1 = srcTones.begin()+t1;
                        Tones::iterator it2 = dstTones.begin()+t2;
                        *it2 = *it1;
                    }
                }
                std::cerr << std::endl;
            }
        }
    }
    
    static SyxBuffer WriteSyx(Patches& patches, Tones& tones)
    {
        unsigned char p=0, t=0;
        SyxBuffer syx;
        for (Patches::iterator it=patches.begin(); it!=patches.end(); ++it)
        {
            *(it->begin()+3) = (unsigned char)0; // set midi channel to 1
            *(it->begin()+8) = p++; // important! renumber the output patches
            syx.insert(syx.end(),it->begin(),it->end());
        }
        for (Tones::iterator it=tones.begin(); it!=tones.end(); ++it)
        {
            SyxBuffer tone(it->begin(),it->end());
            if (tone.size() == 67)
            {
                // convert PGR, APR, IPR tones to BLD
                tone.insert(tone.begin()+7,0);
                tone.insert(tone.begin()+7,0);
            }
            tone[2] = 0x37; // BLD
            tone[3] = 0; // set midi channel to 1
            tone[8] = t++; // important! renumber the output patches
            syx.insert(syx.end(),tone.begin(),tone.end());
        }
        return syx;
    }
    
    static void FixNameNumbers(std::string& name)
    {
        for (std::string::iterator it = name.begin(); it != name.end(); ++it)
            if (*it < 10)
                *it += 48;
    }
    
    static void PrintPatchName(const SyxBuffer& patch, unsigned char p)
    {
        std::cerr << " " << (char)(65+p/8) << p%8+1 << "  ";
        std::string patchName = GetPatchName(patch);
        unsigned char a,b;
        GetPatchTones(patch,a,b);
        std::cerr << patchName << "    " << std::setfill(' ') << std::setw(3) << (int)a+1 << "  " << std::setw(3) << (int)b+1;
    }

    static void PrintToneName(const SyxBuffer& tone, unsigned char t)
    {
        std::string toneName(tone.begin()+9,tone.begin()+19);
        FixNameNumbers(toneName);
        std::cerr << std::setfill('0') << std::setw(2) << t+1 << "  " << toneName;
    }
    
    static void PrintPatchesAndTones(const Patches& patches, const Tones& tones, const bool printPresets)
    {
        std::cerr << "-----------------------------------" << std::endl;
        std::cerr << "PATCH      NAME              A    B" << std::endl;
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
    
    static void PrintUnusedTones(const ToneNumberSet& usedTones, const bool includePresets)
    {
        std::cerr << "User tones not in use: ";
        ToneNumberSet unusedTones = InvertToneNumberSet(usedTones);
        for (ToneNumberSet::const_iterator it=unusedTones.begin(); it!=unusedTones.end(); ++it)
        {
            int tone = *it+1;
            if (tone<=50)
                std::cerr << tone << " ";
        }
        if (includePresets)
        {
            std::cerr << std::endl << "Factory preset tones not in use: ";
            for (ToneNumberSet::const_iterator it=unusedTones.begin(); it!=unusedTones.end(); ++it)
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
optional arguments (addresses for patches: a1-h8 or A1-H8; user tones: 1-50):\n\
  -h                                  show this help message and exit\n\
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
                                      and do not overwrite any unused tones\n\
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
            
            Swaps swaps;

            Swaps copies;
            Patches srcPatches;
            Tones srcTones;

            bool print = false;
            bool printUsed = false;
            bool printUnused = false;
            bool includePresets = false;
            bool forceAddressPair = false;
            bool forceCopy = false;
            bool overwriteUnused = true;
            
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
            }
            
            SwapPatches(swaps.first,patches);

            Patches noPatches;
            SwapTones(swaps.second,forceAddressPair?noPatches:patches,tones);
            
            CopyPatchesAndTones(copies,overwriteUnused,forceCopy,srcPatches,srcTones,patches,tones);
            
            syx = WriteSyx(patches,tones);
            
            // if there's nothing else to do then print
            if (swaps.first.empty() && swaps.second.empty() && copies.first.empty() && copies.second.empty())
                print = true;
            
            if (print)
            {
                patches = ParsePatches(syx);
                tones = ParseTones(syx);
                PrintPatchesAndTones(patches,tones,includePresets);
            }

            ToneNumberSet usedTones = ParseUsedTones(patches);
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


