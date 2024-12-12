#ifndef LUAUPI_FS_H
#define LUAUPI_FS_H

#include <string>

namespace LuauPi
{

class FS
{
public:
	static std::string read_file(const std::string& filepath);
};

}

#endif
