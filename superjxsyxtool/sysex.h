//
//  sysex.h
//  superjxsyxtool
//
//  Created by Christian E. on 2017/Oct/19.
//  Copyright © 2017 Ten by Ten. All rights reserved.
//

#ifndef sysex_h
#define sysex_h

#include <stdint.h>
#include <vector>

typedef std::vector<uint8_t> SyxBuffer;

inline uint8_t CombineBytes(uint8_t top, uint8_t bot)
{
	top = top << 4;
	if (top > 127) top -= 128;
	return top | bot;
}

inline uint8_t Decode2valParameter(uint8_t val)
{
	return val>=64;
}

inline uint8_t Decode3valParameter(uint8_t val)
{
	if (val<32)
		return 0;
	if (val<64)
		return 1;
	return 2;
}

inline uint8_t Decode4valParameter(uint8_t val)
{
	if (val<32)
		return 0;
	if (val<64)
		return 1;
	if (val<96)
		return 2;
	return 3;
}

inline uint8_t Decode100valParameter(uint8_t val)
{
	return static_cast<uint8_t>(val * .78f);
}


#endif /* sysex_h */
