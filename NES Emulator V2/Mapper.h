#pragma once
#include <cstdint>

enum MIRROR
{
	HARDWARE,
	HORIZONTAL,
	VERTICAL,
	ONESCREEN_LO,
	ONESCREEN_HI,
};

class Mapper
{
public:
	Mapper(uint8_t prgBanks, uint8_t chrBanks);
	virtual ~Mapper();

public:
	// Transform CPU bus address into PRG ROM offset and possibly return data
	virtual bool cpuMapRead(uint16_t addr, uint32_t& mapped_addr, uint8_t& data) = 0;
	virtual bool cpuMapWrite(uint16_t addr, uint32_t& mapped_addr, uint8_t data = 0) = 0;

	// Transform PPU bus address into CHR ROM offset
	virtual bool ppuMapRead(uint16_t addr, uint32_t& mapped_addr) = 0;
	virtual bool ppuMapWrite(uint16_t addr, uint32_t& mapped_addr) = 0;

	// Reset mapper to known state
	virtual void reset() = 0;

	// Additional virtual functions
	virtual MIRROR mirror();
	virtual bool irqState();
	virtual void irqClear();
	virtual void scanline();

protected:
	uint8_t nPRGBanks = 0;
	uint8_t nCHRBanks = 0;
};
