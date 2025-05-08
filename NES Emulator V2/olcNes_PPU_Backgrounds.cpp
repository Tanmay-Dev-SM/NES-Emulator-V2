#include <iostream>
#include <sstream>
#include <vector>
#include <memory>

#include "Bus.h"
#include "cpu6502.h"

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

//#define DEBUG_UI  // Uncomment to enable debug UI

class Demo_olc2C02 : public olc::PixelGameEngine
{
public:
	Demo_olc2C02() {
		sAppName = "NES Emulator";
	}

	void SetCart(const std::shared_ptr<Cartridge>& selectedCart) {
		cart = selectedCart;
	}

private:
	Bus nes;
	std::shared_ptr<Cartridge> cart;
	bool bEmulationRun = false;
	float fResidualTime = 0.0f;
	uint8_t nSelectedPalette = 0x00;

	std::map<uint16_t, std::string> mapAsm;

	std::string hex(uint32_t n, uint8_t d)
	{
		std::string s(d, '0');
		for (int i = d - 1; i >= 0; i--, n >>= 4)
			s[i] = "0123456789ABCDEF"[n & 0xF];
		return s;
	};

	void DrawCpu(int x, int y)
	{
		DrawString(x, y, "STATUS:", olc::WHITE);
		DrawString(x + 64, y, "N", nes.cpu.status & cpu6502::N ? olc::GREEN : olc::RED);
		DrawString(x + 80, y, "V", nes.cpu.status & cpu6502::V ? olc::GREEN : olc::RED);
		DrawString(x + 96, y, "-", nes.cpu.status & cpu6502::U ? olc::GREEN : olc::RED);
		DrawString(x + 112, y, "B", nes.cpu.status & cpu6502::B ? olc::GREEN : olc::RED);
		DrawString(x + 128, y, "D", nes.cpu.status & cpu6502::D ? olc::GREEN : olc::RED);
		DrawString(x + 144, y, "I", nes.cpu.status & cpu6502::I ? olc::GREEN : olc::RED);
		DrawString(x + 160, y, "Z", nes.cpu.status & cpu6502::Z ? olc::GREEN : olc::RED);
		DrawString(x + 178, y, "C", nes.cpu.status & cpu6502::C ? olc::GREEN : olc::RED);
		DrawString(x, y + 10, "PC: $" + hex(nes.cpu.pc, 4));
		DrawString(x, y + 20, "A: $" + hex(nes.cpu.a, 2) + "  [" + std::to_string(nes.cpu.a) + "]");
		DrawString(x, y + 30, "X: $" + hex(nes.cpu.x, 2) + "  [" + std::to_string(nes.cpu.x) + "]");
		DrawString(x, y + 40, "Y: $" + hex(nes.cpu.y, 2) + "  [" + std::to_string(nes.cpu.y) + "]");
		DrawString(x, y + 50, "Stack P: $" + hex(nes.cpu.stkp, 4));
	}

	bool OnUserCreate()
	{
		if (!cart || !cart->ImageValid())
		{
			std::cerr << "Failed to load cartridge.\n";
			return false;
		}

		nes.insertCartridge(cart);
		mapAsm = nes.cpu.disassemble(0x0000, 0xFFFF);
		nes.reset();
		return true;
	}

	bool OnUserUpdate(float fElapsedTime)
	{
		if (GetKey(olc::Key::ESCAPE).bPressed)
			return false;

		Clear(olc::BLACK);

		nes.controller[0] = 0x00;
		nes.controller[0] |= GetKey(olc::Key::X).bHeld ? 0x80 : 0x00;
		nes.controller[0] |= GetKey(olc::Key::Z).bHeld ? 0x40 : 0x00;
		nes.controller[0] |= GetKey(olc::Key::A).bHeld ? 0x20 : 0x00;
		nes.controller[0] |= GetKey(olc::Key::S).bHeld ? 0x10 : 0x00;
		nes.controller[0] |= GetKey(olc::Key::UP).bHeld ? 0x08 : 0x00;
		nes.controller[0] |= GetKey(olc::Key::DOWN).bHeld ? 0x04 : 0x00;
		nes.controller[0] |= GetKey(olc::Key::LEFT).bHeld ? 0x02 : 0x00;
		nes.controller[0] |= GetKey(olc::Key::RIGHT).bHeld ? 0x01 : 0x00;

		if (GetKey(olc::Key::SPACE).bPressed) bEmulationRun = !bEmulationRun;
		if (GetKey(olc::Key::R).bPressed) nes.reset();
		if (GetKey(olc::Key::P).bPressed) (++nSelectedPalette) &= 0x07;

		if (bEmulationRun)
		{
			if (fResidualTime > 0.0f)
				fResidualTime -= fElapsedTime;
			else
			{
				fResidualTime += (1.0f / 60.0f) - fElapsedTime;
				do { nes.clock(); } while (!nes.ppu.frame_complete);
				nes.ppu.frame_complete = false;
			}
		}
		else
		{
			if (GetKey(olc::Key::C).bPressed)
			{
				do { nes.clock(); } while (!nes.cpu.complete());
				do { nes.clock(); } while (nes.cpu.complete());
			}
			if (GetKey(olc::Key::F).bPressed)
			{
				do { nes.clock(); } while (!nes.ppu.frame_complete);
				do { nes.clock(); } while (!nes.cpu.complete());
				nes.ppu.frame_complete = false;
			}
		}

#ifdef DEBUG_UI
		DrawCpu(516, 2);
		for (int i = 0; i < 26; i++)
		{
			std::string s = hex(i, 2) + ": (" + std::to_string(nes.ppu.pOAM[i * 4 + 3])
				+ ", " + std::to_string(nes.ppu.pOAM[i * 4 + 0]) + ") "
				+ "ID: " + hex(nes.ppu.pOAM[i * 4 + 1], 2) +
				+" AT: " + hex(nes.ppu.pOAM[i * 4 + 2], 2);
			DrawString(516, 72 + i * 10, s);
		}

		const int nSwatchSize = 6;
		for (int p = 0; p < 8; p++)
			for (int s = 0; s < 4; s++)
				FillRect(516 + p * (nSwatchSize * 5) + s * nSwatchSize, 340,
					nSwatchSize, nSwatchSize, nes.ppu.GetColourFromPaletteRam(p, s));

		DrawRect(516 + nSelectedPalette * (nSwatchSize * 5) - 1, 339, (nSwatchSize * 4), nSwatchSize, olc::WHITE);
		DrawSprite(516, 348, &nes.ppu.GetPatternTable(0, nSelectedPalette));
		DrawSprite(648, 348, &nes.ppu.GetPatternTable(1, nSelectedPalette));
#endif
		DrawSprite(0, 0, &nes.ppu.GetScreen(), 2);
		return true;
	}
};

int main()
{
	std::vector<std::string> romList = {
		"Adventure Island (USA)", "Adventure Island II (USA)", "Balloon Fight (USA)",
		"Contra (USA)", "DonkeyKong", "Dragon Ball Z - Super Butouden 2 (FR)",
		"DuckTales (USA)", "Excitebike (USA) (e-Reader Edition)", "Ice Climber (Japan) (En)",
		"Kirby's Adventure (USA)", "Kung Fu (Japan, USA) (En)", "Mega Man 2 (USA)",
		"nestest", "Ninja Gaiden III - The Ancient Ship of Doom (USA)", "Prince of Persia (USA)",
		"Pro Wrestling (USA)", "Spider-Man - Return of the Sinister Six (USA)",
		"Super Mario Bros. (World)", "Top Gun (USA)"
	};

	std::cout << "Select a ROM to play:\n";
	std::cout << "Put the number and press \"enter key\" to load\n";
	std::cout << "  SPACE = Pause/Run | R = Reset | esc = Quit\n\n";
	for (size_t i = 0; i < romList.size(); ++i)
		std::cout << i + 1 << ". " << romList[i] << "\n";

	int choice = 0;
	std::cin >> choice;
	if (choice < 1 || choice >(int)romList.size()) {
		std::cerr << "Invalid choice.\n";
		return 1;
	}

	std::string selectedRom = "roms/" + romList[choice - 1] + ".nes";
	std::cout << "\nLaunching \"" << romList[choice - 1] << "\"...\n";

	// Ask for fullscreen
	char fs;
	std::cout << "\nEnable Fullscreen? (y/n): ";
	std::cin >> fs;
	bool fullscreen = (fs == 'y' || fs == 'Y');

	std::cout << "\nControls:\n";
	std::cout << "  X = A   | Z = B   | A = Select | S = Start\n";
	std::cout << "  Arrow Keys = D-Pad\n";
	std::cout << "  SPACE = Pause/Run | R = Reset\n\n";

	Demo_olc2C02 demo;
	demo.SetCart(std::make_shared<Cartridge>(selectedRom));

#ifdef DEBUG_UI
	demo.Construct(780, 480, 2, 2, fullscreen, true);
#else
	demo.Construct(480, 480, 2, 2, fullscreen, true);
#endif

	demo.Start();
	return 0;
}