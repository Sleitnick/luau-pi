#include "state.h"

#include <lualib.h>
#include <luacodegen.h>

#include "scheduler.h"
#include "pilib.h"
#include "tasklib.h"
#include "threaddata.h"

using namespace LuauPi;

static void user_thread(lua_State* parent, lua_State* L)
{
	if (parent)
	{
		ThreadData* td = new ThreadData();
		lua_setthreaddata(L, td);
	}
	else
	{
		ThreadData* td = static_cast<ThreadData*>(lua_getthreaddata(L));
		if (td)
		{
			delete td;
		}
	}
}

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

	lua_callbacks(L)->userdata = reinterpret_cast<void*>(&user_thread);
}

LuauState::~LuauState()
{
	lua_close(L);
}

lua_State* LuauState::get()
{
	return L;
}
