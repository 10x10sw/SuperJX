//
//  patch.h
//  superjxsyxtool
//
//  Created by Christian E. on 2017/Oct/19.
//  Copyright © 2017 Ten by Ten. All rights reserved.
//

#ifndef patch_h
#define patch_h

#include <string>
#include "sysex.h"

struct SuperJXPatch
{
	SuperJXPatch(const SyxBuffer& buffer);

	void Print() const;

	static std::string GetName(const SyxBuffer& patch);
	
    static std::string GetPatchAddress(uint8_t p);

    static void GetTones(const SyxBuffer& patch, uint8_t& a, uint8_t& b, bool& holda, bool& holdb);
	
	static void SetTones(SyxBuffer& patch, uint8_t a, uint8_t b, bool holda, bool holdb);
	
	enum
	{
		kPatchSize = 106,
	};
	
	uint8_t number;
	std::string name;
	
	uint8_t a_b_balance;
	uint8_t dual_detune;
	uint8_t upper_split_point;
	uint8_t lower_split_point;
	uint8_t portamento_time;
	uint8_t bend_range;
	uint8_t key_mode;
	uint8_t total_volume;
	uint8_t aftertouch_vibrato;
	uint8_t aftertouch_brightness;
	uint8_t aftertouch_volume;
	
	uint8_t tone_a;
	int8_t tone_a_chromatic_shift;
	uint8_t tone_a_key_assign;
	int8_t tone_a_unison_detune;
	bool tone_a_hold;
	uint8_t tone_a_lfo_mod_depth;
	bool tone_a_portamento;
	bool tone_a_bender;
	
	uint8_t tone_b;
	int8_t tone_b_chromatic_shift;
	uint8_t tone_b_key_assign;
	int8_t tone_b_unison_detune;
	bool tone_b_hold;
	uint8_t tone_b_lfo_mod_depth;
	bool tone_b_portamento;
	bool tone_b_bender;
	
	uint8_t chase_play_level;
	uint8_t chase_play_mode;
	uint8_t chase_play_time;
	bool chase_play_switch;
};

#endif /* patch_h */
