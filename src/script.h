#ifndef SCRIPT_H
#define SCRIPT_H

#include <lua.h>
#include <string>

class LuauScript
{
public:
	static lua_State* load_and_run(lua_State* L, const std::string& filepath, int* status);
};

#endif
