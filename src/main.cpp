#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <iterator>
#include <algorithm>
#include <cstdlib>
#include <cstring>

extern "C"
{
#include "../libz80/z80.h"
}

bool verbose = true;
bool debug_read = false;

std::array<uint8_t, 64 * 1024> address_space{0};

#define SYS_ROM_LOCATION	0x0000
#define USER_ROM_LOCATION	0x2000
#define UART_LOCATION		0x4000
#define RAM_LOCATION		0x8000


enum class rom
{
	system, user
};

void install_rom(rom type, const uint8_t* data)
{
	uint16_t offset = 0;
	switch(type)
	{
	case rom::system:
		offset += SYS_ROM_LOCATION;
		break;
	case rom::user:
		offset += USER_ROM_LOCATION;
		break;
	}

	std::memcpy(address_space.data() + offset, data, 8 * 1024);
}

void loadRoms(const std::string& sys_binary, const std::string& user_binary)
{
	std::cout << "Loading roms : " << sys_binary << ' ' << user_binary << '\n';
	std::array<uint8_t, 8 * 1024> rom{0};
	{ //input file context
		std::ifstream sys_rom_input_stream(sys_binary, std::ios::binary);

		std::copy(std::istreambuf_iterator<char>(sys_rom_input_stream), std::istreambuf_iterator<char>(), rom.begin());
	} //close file


	//copy the 8k in the address space
	install_rom(rom::system, rom.data());


	//fill array with 0s
	std::generate(rom.begin(), rom.end(), [] {return 0; });

	if (user_binary.empty()) return;

	{ //input file context
		std::ifstream sys_rom_input_stream(user_binary, std::ios::binary);

		std::copy(std::istreambuf_iterator<char>(sys_rom_input_stream), std::istreambuf_iterator<char>(), rom.begin());
	} //close file

	install_rom(rom::user, rom.data());
}

enum class device
{
	rom, ram, serial, other
};

device checkDevice(uint16_t addr)
{
	if (addr >= SYS_ROM_LOCATION && addr < SYS_ROM_LOCATION + 8 * 1024)
		return device::rom;
	if (addr >= USER_ROM_LOCATION && addr < USER_ROM_LOCATION + 8 * 1024)
		return device::rom;
	if (addr >= UART_LOCATION && addr < UART_LOCATION + 8)
		return device::serial;
	if (addr >= RAM_LOCATION && addr < RAM_LOCATION + 32 * 1024)
		return device::ram;

	return device::other;
}

byte bus_read(int param, ushort addr)
{
	if (verbose&&!debug_read) fprintf(stderr, "cpu reading location %4X\n", addr);
	switch (checkDevice(addr))
	{
	case device::rom:
	case device::ram:
		return address_space[addr];
	case device::serial:
		if(!debug_read)
			throw std::runtime_error("serial i/o not implemented yet");
		break;
	case device::other:
		if(!debug_read)
		throw std::runtime_error("No RED_80 expansion support on the emulator (yet)");
	}

	return 0x00;
}

void bus_write(int param, ushort addr, byte data)
{
	if (verbose) fprintf(stderr, "cpu writing %2X to location %4X\n", data, addr);
	switch(checkDevice(addr))
	{
	case device::rom:
		throw std::runtime_error("cannot write to ROM!");
	case device::ram:
		address_space[addr] = data;
		break;
	case device::serial:
		//TODO emulate serial interface
		throw std::runtime_error("did not emulate serial interface (yet)");
		break;
	case device::other:
		//TODO implement expansion emulation
		throw std::runtime_error("No RED_80 expansion support on the emulator (yet)");
	}
}


int main(int argc, char** argv)
{
	std::string path_to_system_rom = "test_rom/nop.bin";
	std::string path_to_user_rom = "";
	char debug_decode[256];

	if (argc >= 2)
		path_to_system_rom = argv[1];
	if (argc >= 3)
		path_to_user_rom = argv[2];

	loadRoms(path_to_system_rom, path_to_user_rom);

	Z80Context ctx;
	ctx.memRead = bus_read;
	ctx.memWrite = bus_write;
	//TODO I don't know a thing about the z80 I/O read/write scheme.
	//ctx.ioRead = bus_read;
	//ctx.ioWrite = bus_write;

	Z80RESET(&ctx);

	bool emulate = true;
	while(emulate)
	{
		debug_read = true;
		Z80Debug(&ctx, nullptr, debug_decode);
		std::cerr << "next CPU instruction " << debug_decode << '\n';
		debug_read = false;
		try
		{
			Z80Execute(&ctx);
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what();
			emulate = false;
#ifdef _DEBUG
			throw;
#endif
		}
	}

	return 0;
}
