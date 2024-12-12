#ifndef LUAUPI_STATE_H
#define LUAUPI_STATE_H

#include <lua.h>

namespace LuauPi
{

class LuauState
{
private:
	lua_State* L;

public:
	LuauState();
	~LuauState();

	lua_State* get();
};

}

#endif
