//
//  tone.h
//  superjxsyxtool
//
//  Created by Christian E. on 2017/Oct/19.
//  Copyright © 2017 Ten by Ten. All rights reserved.
//

#ifndef tone_h
#define tone_h

#include <string>
#include "sysex.h"

struct SuperJXTone
{
	SuperJXTone(const SyxBuffer& buffer);

	void Print() const;
	
	static std::string GetName(const SyxBuffer& tone);

	enum
	{
		kToneSize = 69,
		kToneSize_Short = 67,
	};
	
	std::string name;

	uint8_t dco1_range;
	uint8_t dco1_waveform;
	int8_t dco1_tune;
	uint8_t dco1_lfo_mod_depth;
	uint8_t dco1_env_mod_depth;

	uint8_t dco2_range;
	uint8_t dco2_waveform;
	uint8_t dco2_xmod;
	int8_t dco2_tune;
	int8_t dco2_fine_tune;
	uint8_t dco2_lfo_mod_depth;
	uint8_t dco2_env_mod_depth;

	uint8_t dco_dynamics;
	uint8_t dco_env_mode;

	uint8_t mixer_dco1;
	uint8_t mixer_dco2;
	uint8_t mixer_env_mod_depth;
	uint8_t mixer_dynamics;
	uint8_t mixer_env_mode;

	uint8_t hpf_cutoff_freq;
	uint8_t vcf_cutoff_freq;
	uint8_t vcf_resonance;
	uint8_t vcf_lfo_mod_depth;
	uint8_t vcf_env_mod_depth;
	uint8_t vcf_key_follow;
	uint8_t vcf_dynamics;
	uint8_t vcf_env_mode;

	uint8_t vca_level;
	uint8_t vca_dynamics;
	uint8_t chorus;

	uint8_t lfo_waveform;
	uint8_t lfo_delay_time;
	uint8_t lfo_rate;

	uint8_t env1_attack;
	uint8_t env1_decay;
	uint8_t env1_sustain;
	uint8_t env1_release;
	uint8_t env1_key_follow;

	uint8_t env2_attack;
	uint8_t env2_decay;
	uint8_t env2_sustain;
	uint8_t env2_release;
	uint8_t env2_key_follow;

	uint8_t vca_env_mode;
};

#endif /* tone_h */
