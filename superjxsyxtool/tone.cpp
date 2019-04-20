//
//  tone.cpp
//  superjxsyxtool
//
//  Created by Christian E. on 2017/Oct/20.
//  Copyright © 2017 Ten by Ten. All rights reserved.
//

#include "tone.h"
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
			name = 9,
			start_of_data = 19,
			
			dco1_range = start_of_data + 1,
			dco1_waveform,
			dco1_tune,
			dco1_lfo_mod_depth,
			dco1_env_mod_depth,
			
			dco2_range,
			dco2_waveform,
			dco2_xmod,
			dco2_tune,
			dco2_fine_tune,
			dco2_lfo_mod_depth,
			dco2_env_mod_depth,
			
			dco_dynamics = start_of_data + 16,
			dco_env_mode,
			
			mixer_dco1,
			mixer_dco2,
			mixer_env_mod_depth,
			mixer_dynamics,
			mixer_env_mode,
			
			hpf_cutoff_freq,
			vcf_cutoff_freq,
			vcf_resonance,
			vcf_lfo_mod_depth,
			vcf_env_mod_depth,
			vcf_key_follow,
			vcf_dynamics,
			vcf_env_mode,
			
			vca_level,
			vca_dynamics,
			chorus,
			
			lfo_waveform,
			lfo_delay_time,
			lfo_rate,
			
			env1_attack,
			env1_decay,
			env1_sustain,
			env1_release,
			env1_key_follow,
			
			env2_attack,
			env2_decay,
			env2_sustain,
			env2_release,
			env2_key_follow,
			
			vca_env_mode = start_of_data + 49,
		};
	}

	namespace Length
	{
		enum
		{
			name = 10,
		};
	}
	
	static uint8_t DecodeDCORange(uint8_t val)
	{
		if (val<32)
			return 16;
		if (val<64)
			return 8;
		if (val<96)
			return 4;
		return 2;
	}
	
	static int8_t DecodeTune(uint8_t val)
	{
		return static_cast<int8_t>(val * 0.189f - 12);
	}
	
	static int8_t DecodeFineTune(uint8_t val)
	{
		return static_cast<int8_t>(val * .789f - 50); // TODO needs work to match the hardware
	}
	
	static void DecodeSyxBuffer(const SyxBuffer& buffer, SuperJXTone& tone)
	{
		tone.dco1_range = DecodeDCORange(buffer[Offset::dco1_range]);
		tone.dco1_waveform = Decode4valParameter(buffer[Offset::dco1_waveform]);
		tone.dco1_tune = DecodeTune(buffer[Offset::dco1_tune]);
		tone.dco1_lfo_mod_depth = Decode100valParameter(buffer[Offset::dco1_lfo_mod_depth]);
		tone.dco1_env_mod_depth = Decode100valParameter(buffer[Offset::dco1_env_mod_depth]);
		
		tone.dco2_range = DecodeDCORange(buffer[Offset::dco2_range]);
		tone.dco2_waveform = Decode4valParameter(buffer[Offset::dco2_waveform]);
		tone.dco2_xmod = Decode4valParameter(buffer[Offset::dco2_xmod]);
		tone.dco2_tune = DecodeTune(buffer[Offset::dco2_tune]);
		tone.dco2_fine_tune = DecodeFineTune(buffer[Offset::dco2_fine_tune]);
		tone.dco2_lfo_mod_depth = Decode100valParameter(buffer[Offset::dco2_lfo_mod_depth]);
		tone.dco2_env_mod_depth = Decode100valParameter(buffer[Offset::dco2_env_mod_depth]);

		tone.dco_dynamics = Decode4valParameter(buffer[Offset::dco_dynamics]);
		tone.dco_env_mode = Decode4valParameter(buffer[Offset::dco_env_mode]);
		
		tone.mixer_dco1 = Decode100valParameter(buffer[Offset::mixer_dco1]);
		tone.mixer_dco2 = Decode100valParameter(buffer[Offset::mixer_dco2]);
		tone.mixer_env_mod_depth = Decode100valParameter(buffer[Offset::mixer_env_mod_depth]);
		tone.mixer_dynamics = Decode4valParameter(buffer[Offset::mixer_dynamics]);
		tone.mixer_env_mode = Decode4valParameter(buffer[Offset::mixer_env_mode]);

		tone.hpf_cutoff_freq = Decode4valParameter(buffer[Offset::hpf_cutoff_freq]);
		tone.vcf_cutoff_freq = Decode100valParameter(buffer[Offset::vcf_cutoff_freq]);
		tone.vcf_resonance = Decode100valParameter(buffer[Offset::vcf_resonance]);
		tone.vcf_lfo_mod_depth = Decode100valParameter(buffer[Offset::vcf_lfo_mod_depth]);
		tone.vcf_env_mod_depth = Decode100valParameter(buffer[Offset::vcf_env_mod_depth]);
		tone.vcf_key_follow = Decode100valParameter(buffer[Offset::vcf_key_follow]);
		tone.vcf_dynamics = Decode4valParameter(buffer[Offset::vcf_dynamics]);
		tone.vcf_env_mode = Decode4valParameter(buffer[Offset::vcf_env_mode]);

		tone.vca_level = Decode100valParameter(buffer[Offset::vca_level]);
		tone.vca_dynamics = Decode4valParameter(buffer[Offset::vca_dynamics]);
		tone.chorus = Decode3valParameter(buffer[Offset::chorus]);

		tone.lfo_waveform = Decode3valParameter(buffer[Offset::lfo_waveform]);
		tone.lfo_delay_time = Decode100valParameter(buffer[Offset::lfo_delay_time]);
		tone.lfo_rate = Decode100valParameter(buffer[Offset::lfo_rate]);
		
		tone.env1_attack = Decode100valParameter(buffer[Offset::env1_attack]);
		tone.env1_decay = Decode100valParameter(buffer[Offset::env1_decay]);
		tone.env1_sustain = Decode100valParameter(buffer[Offset::env1_sustain]);
		tone.env1_release = Decode100valParameter(buffer[Offset::env1_release]);
		tone.env1_key_follow = Decode4valParameter(buffer[Offset::env1_key_follow]);

		tone.env2_attack = Decode100valParameter(buffer[Offset::env2_attack]);
		tone.env2_decay = Decode100valParameter(buffer[Offset::env2_decay]);
		tone.env2_sustain = Decode100valParameter(buffer[Offset::env2_sustain]);
		tone.env2_release = Decode100valParameter(buffer[Offset::env2_release]);
		tone.env2_key_follow = Decode4valParameter(buffer[Offset::env2_key_follow]);

		tone.vca_env_mode = Decode2valParameter(buffer[Offset::vca_env_mode]);
	}
	
	static std::string StringForWaveform(uint8_t val)
	{
		switch (val)
		{
			case 0: return "Noise   ";
			case 1: return "Square  ";
			case 2: return "Pulse   ";
			case 3: return "Sawtooth";
		}
		return "";
	}

	static std::string StringForLFOWaveform(uint8_t val)
	{
		switch (val)
		{
			case 0: return "Random  ";
			case 1: return "Square  ";
			case 2: return "Triangle";
		}
		return "";
	}
	
	static std::string StringForEnvMode(uint8_t val)
	{
		switch (val)
		{
			case 0: return "Env2 Inverted (u-2)";
			case 1: return "Env2 Normal   (n-2)";
			case 2: return "Env1 Inverted (u-1)";
			case 3: return "Env1 Normal   (n-1)";
		}
		return "";
	}

	static std::string StringForVCAEnvMode(uint8_t val)
	{
		return val ? "Env2" : "Gate";
	}

	static std::string StringForIntWithOff(uint8_t val)
	{
		if (val==0)
			return "Off";
		char buf[8];
		sprintf(buf,"%2d ",val);
		return buf;
	}

	static std::string StringForXMod(uint8_t val)
	{
		switch (val)
		{
			case 0: return "Off   ";
			case 1: return "Sync 1";
			case 2: return "Sync 2";
			case 3: return "XMod  ";
		}
		return "";
	}
}

SuperJXTone::SuperJXTone(const SyxBuffer& buffer)
{
	if (buffer.size()>=kToneSize_Short)
	{
		name = GetName(buffer);
		DecodeSyxBuffer(buffer, *this);
	}
}

void SuperJXTone::Print() const
{
	std::cerr << "----------------" << std::endl;
	std::cerr << "TONE: " << name << std::endl;
	
	std::cerr << std::setfill(' ');
	
	std::cerr << "DCO1:  Range: " << std::setw(2) << (int)dco1_range << "\'    Waveform:  " << StringForWaveform(dco1_waveform) << std::endl;
	std::cerr << "        Tune: " << std::setw(2) << std::showpos << (int)dco1_tune << std::noshowpos << std::endl;
	std::cerr << "     LFO Mod: " << std::setw(2) << (int)dco1_lfo_mod_depth << "      Env Mod: " << std::setw(2) << (int)dco1_env_mod_depth << std::endl;

	std::cerr << "DCO2:  Range: " << std::setw(2) << (int)dco2_range << "\'    Waveform: " << StringForWaveform(dco2_waveform) << "   XMod: " << StringForXMod(dco2_xmod) << std::endl;
	std::cerr << "        Tune: " << std::setw(2) << std::showpos << (int)dco2_tune << "    Fine Tune: " << std::setw(2) << (int)dco2_fine_tune << std::noshowpos << std::endl;
	std::cerr << "     LFO Mod: " << std::setw(2) << (int)dco2_lfo_mod_depth << "      Env Mod: " << std::setw(2) << (int)dco2_env_mod_depth << std::endl;

	std::cerr << "DCO Dynamics: " << std::setw(2) << (int)dco_dynamics << "     Env Mode: " << StringForEnvMode(dco_env_mode) << std::endl;

	std::cerr << "Mixer:  DCO1: " << std::setw(2) << (int)mixer_dco1 << "         DCO2: " << std::setw(2) << (int)mixer_dco2 << std::endl;
	std::cerr << "     Env Mod: " << std::setw(2) << (int)mixer_env_mod_depth << "     Dynamics: " << std::setw(2) << (int)mixer_dynamics  << "     Env Mode: " << StringForEnvMode(mixer_env_mode) << std::endl;

	std::cerr << "HPF:  Cutoff: " << std::setw(2) << (int)hpf_cutoff_freq << std::endl;
	std::cerr << "VCF:  Cutoff: " << std::setw(2) << (int)vcf_cutoff_freq << "    Resonance: " << std::setw(2) << (int)vcf_resonance << std::endl;
	std::cerr << "     LFO Mod: " << std::setw(2) << (int)vcf_lfo_mod_depth << "      Env Mod: " << std::setw(2) << (int)vcf_env_mod_depth << std::endl;
	std::cerr << "  Key Follow: " << std::setw(2) << (int)vcf_key_follow << "     Dynamics: " << std::setw(2) << (int)vcf_dynamics << "     Env Mode: " << StringForEnvMode(vcf_env_mode) << std::endl;

	std::cerr << "VCA:   Level: " << std::setw(2) << (int)vca_level << "         Mode: " << StringForVCAEnvMode(vca_env_mode) << "   Dynamics: " << std::setw(2) << (int)vca_dynamics<< "       Chorus: " << StringForIntWithOff(chorus) << std::endl;

	std::cerr << "LFO Waveform: " << StringForLFOWaveform(lfo_waveform) << "  Delay: " << std::setw(2) << (int)lfo_delay_time << "         Rate: " << std::setw(2) << (int)lfo_rate << std::endl;

	std::cerr << "ENV1: Attack: " << std::setw(2) << (int)env1_attack << "        Decay: " << std::setw(2) << (int)env1_decay << "      Sustain: " << std::setw(2) << (int)env1_sustain << "      Release: " << std::setw(2) << (int)env1_release << std::endl;
	std::cerr << "  Key Follow: " << std::setw(2) << (int)env1_key_follow << std::endl;

	std::cerr << "ENV2: Attack: " << std::setw(2) << (int)env2_attack << "        Decay: " << std::setw(2) << (int)env2_decay << "      Sustain: " << std::setw(2) << (int)env2_sustain << "      Release: " << std::setw(2) << (int)env2_release << std::endl;
	std::cerr << "  Key Follow: " << std::setw(2) << (int)env2_key_follow << std::endl;
}

// static
std::string SuperJXTone::GetName(const SyxBuffer& tone)
{
	std::string name(tone.begin()+Offset::name,tone.begin()+Offset::name+Length::name);
	// fix numbers
	for (std::string::iterator it = name.begin(); it != name.end(); ++it)
		if (*it < 10)
			*it += 48;
	return name;
}
