#include "state.h"

#include <lualib.h>
#include <luacodegen.h>
#include "scheduler.h"
#include "pilib.h"
#include "tasklib.h"

using namespace LuauPi;

LuauState::LuauState() : L(luaL_newstate())
{
	luaL_openlibs(L);
	pilib_open(L);
	task_lib_open(L);
	LuauTaskScheduler::create(L);

	if (luau_codegen_supported())
	{
		luau_codegen_create(L);
	}

	luaL_sandbox(L);
}

LuauState::~LuauState()
{
	lua_close(L);
}

lua_State* LuauState::get()
{
	return L;
}
