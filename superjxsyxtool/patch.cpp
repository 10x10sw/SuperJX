//
//  patch.cpp
//  superjxsyxtool
//
//  Created by Christian E. on 2017/Oct/20.
//  Copyright © 2017 Ten by Ten. All rights reserved.
//

#include "patch.h"
#include <stdint.h>
#include <stdio.h>
#include <iomanip>
#include <iostream>

namespace
{
	namespace Offset
	{
		enum
		{
			a_b_balance = 18,
			dual_detune,
			upper_split_point,
			lower_split_point,
			portamento_time,
			total_volume,
			aftertouch_vibrato,
			aftertouch_brightness,
			aftertouch_volume,

			tone_a = 27,
			tone_a_chromatic_shift,
			tone_a_unison_detune,
			tone_a_lfo_mod_depth,
			tone_a_bender,

			tone_b = 32,
			tone_b_chromatic_shift,
			tone_b_unison_detune,
			tone_b_lfo_mod_depth,
			tone_b_bender,
			
			chase_play_level,
			chase_play_time,

            padded_number = 8,
			padded_name = 9,
			padded_tone_a = 63,
			padded_tone_a_hold = 63,
            padded_tone_a_key_assign = 87,
			padded_tone_a_portamento = 69,
            padded_bend_range_b0 = 45,
            padded_bend_range_b1 = 61,
            padded_bend_range_b2 = 104,
            padded_key_mode_b0 = 49,
            padded_key_mode_b1 = 51,
            padded_key_mode_b2 = 57,
            padded_key_mode_b3 = 59,
			padded_tone_b = 73,
			padded_tone_b_hold = 73,
            padded_tone_b_key_assign = 89,
			padded_tone_b_portamento = 79,
            padded_chase_play_mode_b0 = 83,
            padded_chase_play_mode_b1 = 85,
			padded_chase_play_switch = 99,
		};
	}
	
	namespace Length
	{
		enum
		{
			patch = 106,
		};
	}
	
	static int8_t DecodeChromaticShift(uint8_t val)
	{
		return static_cast<int8_t>(val);
	}
	
	static int8_t DecodeTune(uint8_t val)
	{
		return static_cast<int8_t>(val * 0.787f - 50);
	}
	
	static void DecodeSyxBuffer(const SyxBuffer& paddedBuffer, SuperJXPatch& patch)
	{
		SyxBuffer buffer;
		if (paddedBuffer.size() == Length::patch)
		{
            patch.number = paddedBuffer[Offset::padded_number];
            
			for (int i=Offset::padded_name; i<Length::patch; i+=2)
			{
				buffer.push_back(CombineBytes(paddedBuffer[i], paddedBuffer[i+1]));
				// std::cerr << std::setfill('0') << std::setw(2) << std::hex << (int)paddedBuffer[i] << " "  << std::setw(2) << (int)paddedBuffer[i+1] << " ";
				
			}
			std::cerr << std::endl;
		}
		else
		{
			buffer = paddedBuffer;
		}
		
		patch.a_b_balance = Decode100valParameter(buffer[Offset::a_b_balance]);
		patch.dual_detune = DecodeTune(buffer[Offset::dual_detune]);
		patch.upper_split_point = buffer[Offset::upper_split_point];
		patch.lower_split_point = buffer[Offset::lower_split_point];
		patch.portamento_time = Decode100valParameter(buffer[Offset::portamento_time]);

        uint8_t b0,b1,b2,b3;
        
        b0 = (paddedBuffer[Offset::padded_bend_range_b0] & 0x8) >> 3;
        b1 = (paddedBuffer[Offset::padded_bend_range_b1] & 0x8) >> 2;
        b2 = (paddedBuffer[Offset::padded_bend_range_b2] & 0x1) << 2;
        patch.bend_range = b0 | b1 | b2;
        
        b0 = (paddedBuffer[Offset::padded_key_mode_b0] & 0x8) >> 3;
        b1 = (paddedBuffer[Offset::padded_key_mode_b1] & 0x8) >> 2;
        b2 = (paddedBuffer[Offset::padded_key_mode_b2] & 0x8) >> 1;
        b3 = (paddedBuffer[Offset::padded_key_mode_b3] & 0x8);
        patch.key_mode = b0 | b1 | b2 | b3;
        
		patch.total_volume = Decode100valParameter(buffer[Offset::total_volume]);
		patch.aftertouch_vibrato = Decode100valParameter(buffer[Offset::aftertouch_vibrato]);
		patch.aftertouch_brightness = Decode100valParameter(buffer[Offset::aftertouch_brightness]);
		patch.aftertouch_volume = Decode100valParameter(buffer[Offset::aftertouch_volume]);

		patch.tone_a = buffer[Offset::tone_a];
		patch.tone_a_chromatic_shift = DecodeChromaticShift(buffer[Offset::tone_a_chromatic_shift]);
        patch.tone_a_key_assign = paddedBuffer[Offset::padded_tone_a_key_assign];
		patch.tone_a_unison_detune = DecodeTune(buffer[Offset::tone_a_unison_detune]);
		patch.tone_a_hold = (paddedBuffer[Offset::padded_tone_a_hold] & 0x8) == 0x8;
		patch.tone_a_lfo_mod_depth = Decode100valParameter(buffer[Offset::tone_a_lfo_mod_depth]);
		patch.tone_a_portamento = (paddedBuffer[Offset::padded_tone_a_portamento] & 0x8) == 0x8;
		patch.tone_a_bender = buffer[Offset::tone_a_bender];

 		patch.tone_b = buffer[Offset::tone_b];
		patch.tone_b_chromatic_shift = DecodeChromaticShift(buffer[Offset::tone_b_chromatic_shift]);
        patch.tone_b_key_assign = paddedBuffer[Offset::padded_tone_b_key_assign];
		patch.tone_b_unison_detune = DecodeTune(buffer[Offset::tone_b_unison_detune]);
		patch.tone_b_hold = (paddedBuffer[Offset::padded_tone_b_hold] & 0x8) == 0x8;
		patch.tone_b_lfo_mod_depth = Decode100valParameter(buffer[Offset::tone_b_lfo_mod_depth]);
		patch.tone_b_portamento = (paddedBuffer[Offset::padded_tone_b_portamento] & 0x8) == 0x8;
		patch.tone_b_bender = buffer[Offset::tone_b_bender];

		patch.chase_play_level = Decode100valParameter(buffer[Offset::chase_play_level]);

        b0 = (paddedBuffer[Offset::padded_chase_play_mode_b0] & 0x8) >> 3;
        b1 = (paddedBuffer[Offset::padded_chase_play_mode_b1] & 0x8) >> 2;
        patch.chase_play_mode = b0 | b1;

        patch.chase_play_time = Decode100valParameter(buffer[Offset::chase_play_time]);
		patch.chase_play_switch = (paddedBuffer[Offset::padded_chase_play_switch] & 0x8) == 0x8;
	}
	
	static std::string StringForBool(bool val)
	{
		return val ? "On " : "Off";
	}

	static std::string StringForSplitPoint(uint8_t val)
	{
		const char* note;
		switch (val%12)
		{
			case 0: note = " A"; break;
			case 1: note = "+A"; break;
			case 2: note = " B"; break;
			case 3: note = " C"; break;
			case 4: note = "+C"; break;
			case 5: note = " D"; break;
			case 6: note = "+D"; break;
			case 7: note = " E"; break;
			case 8: note = " F"; break;
			case 9: note = "+F"; break;
			case 10: note = " G"; break;
			case 11: note = "+G"; break;
		}
		uint8_t octave = (val+9)/12;
		char buf[8];
		sprintf(buf,"%s%d",note,octave);
		return buf;
	}
	
    static std::string StringForBendRange(uint8_t val)
    {
        switch (val)
        {
            case 0: return " 2";
            case 1: return " 3";
            case 2: return " 4";
            case 3: return " 7";
            default: return "12";
        }
        return "";
    }
    
	static std::string StringForKeyMode(uint8_t val)
	{
		switch (val)
		{
            default:
            case 0: return "Dual   ";
			case 1: return "A Whole";
			case 2: return "Split  ";
            case 3: return "B Whole";
			case 4: return "T-Voice";
			case 8: return "X-Fade ";
		}
		return "";
	}
	
	static std::string StringForKeyAssign(uint8_t val)
	{
		switch (val)
		{
			default:
			case 1: return "Poly 1  ";
			case 9: return "Poly 2  ";
			case 3: return "Unison 1";
			case 0xb: return "Unison 2";
			case 5: return "Mono 1  ";
			case 0xd: return "Mono 2  ";
		}
		return "";
	}

	static std::string StringForChaseMode(uint8_t val)
	{
		switch (val)
		{
			default:
			case 0: return "A-B  ";
			case 1: return "A-B-A";
            case 2: return "A-B-B";
		}
		return "";
	}
}

SuperJXPatch::SuperJXPatch(const SyxBuffer& buffer)
{
	if (buffer.size()>=kPatchSize)
	{
		name = GetName(buffer);
		GetTones(buffer, tone_a, tone_b, tone_a_hold, tone_b_hold);
		DecodeSyxBuffer(buffer, *this);
	}
}

void SuperJXPatch::Print() const
{
	std::cerr << "----------------" << std::endl;
    std::cerr << "PATCH " << GetPatchAddress(number) << ": " << name << std::endl;
	
	std::cerr << std::setfill(' ');
	
	std::cerr << "        A/B Balance: " << std::setw(2) << (int)a_b_balance << "         Dual Detune: " << std::setw(2) << std::showpos << (int)dual_detune << std::noshowpos << std::endl;
	std::cerr << " Split Point: Upper: " << StringForSplitPoint(upper_split_point) << "              Lower: " << StringForSplitPoint(lower_split_point) <<  std::endl;
	std::cerr << "    Portamento Time: " << std::setw(2) << (int)portamento_time << "          Bend Range: " << StringForBendRange(bend_range) << std::endl;
	std::cerr << "           Key Mode: " << StringForKeyMode(key_mode) << "   Total Volume: " << std::setw(2) << (int)total_volume << std::endl;
	std::cerr << "Aftertouch: Vibrato: " << std::setw(2) << (int)aftertouch_vibrato << "          Brightness: " << std::setw(2) << (int)aftertouch_brightness <<  "     Volume: " << std::setw(2) << (int)aftertouch_volume << std::endl;

	std::cerr << "Tone A (Upper): " << std::setw(2) << (int)tone_a+1 << "          Chromatic Shift: " << std::setw(2) << std::showpos << (int)tone_a_chromatic_shift << std::noshowpos << std::endl;
	std::cerr << "    Key Assign: " << StringForKeyAssign(tone_a_key_assign) << "      Unison Detune: " << std::setw(2) << std::showpos << (int)tone_a_unison_detune << std::noshowpos << std::endl;
	std::cerr << "          Hold: " << StringForBool(tone_a_hold) << "           LFO Mod Depth: " << (int)tone_a_lfo_mod_depth << std::endl;
	std::cerr << "    Portamento: " << StringForBool(tone_a_portamento) << "                  Bender: " << StringForBool(tone_a_bender) << std::endl;

	std::cerr << "Tone B (Lower): " << std::setw(2) << (int)tone_b+1 << "          Chromatic Shift: " << std::setw(2) << std::showpos << (int)tone_b_chromatic_shift << std::noshowpos << std::endl;
	std::cerr << "    Key Assign: " << StringForKeyAssign(tone_b_key_assign) << "      Unison Detune: " << std::setw(2) << std::showpos << (int)tone_b_unison_detune << std::noshowpos << std::endl;
	std::cerr << "          Hold: " << StringForBool(tone_b_hold) << "           LFO Mod Depth: " << (int)tone_b_lfo_mod_depth << std::endl;
	std::cerr << "    Portamento: " << StringForBool(tone_b_portamento) << "                  Bender: " << StringForBool(tone_b_bender) << std::endl;

    std::cerr << "Chase:   Level: " << std::setw(2) << (int)chase_play_level << "     Mode: " << StringForChaseMode(chase_play_mode) << "     Time: " << std::setw(2) << (int)chase_play_time << "     Switch: " << StringForBool(chase_play_switch) << std::endl;
}

//static
std::string SuperJXPatch::GetName(const SyxBuffer& patch)
{
	std::string name;
	for (int i=0; i<18; i++)
	{
		unsigned char c = CombineBytes(patch[Offset::padded_name+i*2], patch[Offset::padded_name+1+i*2]);
		if (c < 10)
			c += 48;
		name.push_back(c);
	}
	return name;
}

//static
std::string SuperJXPatch::GetPatchAddress(uint8_t p)
{
    std::string s;
    s += (char)(65+p/8);
    s += (char)(49+p%8);
    return s;
}

//static
void SuperJXPatch::GetTones(const SyxBuffer& patch, uint8_t& a, uint8_t& b, bool& holda, bool& holdb)
{
	a = CombineBytes(patch[Offset::padded_tone_a],patch[Offset::padded_tone_a+1]);
	b = CombineBytes(patch[Offset::padded_tone_b],patch[Offset::padded_tone_b+1]);
	holda = (patch[Offset::padded_tone_a_hold] & 0x8) == 0x8;
	holdb = (patch[Offset::padded_tone_b_hold] & 0x8) == 0x8;
}

// static
void SuperJXPatch::SetTones(SyxBuffer& patch, uint8_t a, uint8_t b, bool holda, bool holdb)
{
	uint8_t top = a >> 4;
	if (holda) top |= 0x8;
	patch[Offset::padded_tone_a] = top;
	patch[Offset::padded_tone_a+1] = a & 0xf;
	top = b >> 4;
	if (holdb) top |= 0x8;
	patch[Offset::padded_tone_b] = top;
	patch[Offset::padded_tone_b+1] = b & 0xf;
}
