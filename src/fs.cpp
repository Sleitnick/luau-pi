#include "fs.h"

#include <fstream>

using namespace LuauPi;

std::string FS::read_file(const std::string& filepath)
{
	constexpr auto read_size = std::size_t(4096);
	auto stream = std::ifstream(filepath.data());
	stream.exceptions(std::ios_base::badbit);

	auto out = std::string{};
	auto buf = std::string(read_size, '\0');
	while (stream.read(&buf[0], read_size))
	{
		out.append(buf, 0, stream.gcount());
	}
	out.append(buf, 0, stream.gcount());
	
	return out;
}
