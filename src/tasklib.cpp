#include "tasklib.h"

#include <lualib.h>
#include "scheduler.h"

static lua_State* prepare_thread(lua_State* L, int idx, int* ref, int* n_args, bool* created_new_thread)
{
	LuauTaskScheduler* scheduler = LuauTaskScheduler::get(L);

	bool is_fn = lua_isfunction(L, idx);
	bool is_thread = lua_isthread(L, idx);
	luaL_argcheck(L, is_fn || is_thread, idx, "expected function or thread");

	lua_State* T;
	*ref = LUA_NOREF;
	if (is_thread)
	{
		T = lua_tothread(L, idx);
		*created_new_thread = false;
	}
	else
	{
		T = scheduler->create_thread(L);
		*created_new_thread = true;

		// Pin the thread:
		*ref = lua_ref(L, -1);

		lua_pop(L, 1);

		// Push function to the thread:
		lua_xpush(L, T, idx);
	}

	// Push arguments to the thread:
	int top = lua_gettop(L);
	for (int i = idx + 1; i <= top; i++)
	{
		lua_xpush(L, T, i);
	}

	*n_args = top - idx;

	return T;
}

static int task_spawn(lua_State* L)
{
	LuauTaskScheduler* scheduler = LuauTaskScheduler::get(L);

	int ref;
	int n_args;
	bool created_new_thread;
	lua_State* T = prepare_thread(L, 1, &ref, &n_args, &created_new_thread);

	scheduler->spawn(T, L, n_args);

	if (created_new_thread)
	{
		// Push new thread to the top of L stack:
		lua_pushthread(T);
		lua_xmove(T, L, 1);
	}

	// Unpin the thread:
	if (ref != LUA_NOREF)
	{
		lua_unref(L, ref);
	}

	// Return thread:
	return 1;
}

static int task_delay(lua_State* L)
{
	LuauTaskScheduler* scheduler = LuauTaskScheduler::get(L);

	double delay_time = luaL_checknumber(L, 1);

	int ref;
	int n_args;
	bool created_new_thread;
	lua_State* T = prepare_thread(L, 2, &ref, &n_args, &created_new_thread);

	scheduler->delay(T, L, n_args, ref, delay_time, false);

	if (created_new_thread)
	{
		// Push new thread to the top of L stack:
		lua_pushthread(T);
		lua_xmove(T, L, 1);
	}

	// Return thread:
	return 1;
}

static int task_defer(lua_State* L)
{
	LuauTaskScheduler* scheduler = LuauTaskScheduler::get(L);

	int ref;
	int n_args;
	bool created_new_thread;
	lua_State* T = prepare_thread(L, 1, &ref, &n_args, &created_new_thread);

	bool success = scheduler->defer(T, L, n_args, ref);
	if (!success)
	{
		lua_unref(L, ref);
		luaL_error(L, "max defer depth reached");
	}

	if (created_new_thread)
	{
		lua_pushthread(T);
		lua_xmove(T, L, 1);
	}

	return 1;
}

static int task_wait(lua_State* L)
{
	LuauTaskScheduler* scheduler = LuauTaskScheduler::get(L);

	double delay_time = luaL_optnumber(L, 1, 0);

	lua_pushthread(L);
	int ref = lua_ref(L, -1);
	lua_pop(L, 1);

	scheduler->delay(L, L, 0, ref, delay_time, true);

	return lua_yield(L, 1);
}

static int task_cancel(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTHREAD);

	lua_State* T = lua_tothread(L, 1);

	LuauTaskScheduler* scheduler = LuauTaskScheduler::get(L);
	scheduler->cancel(T);

	return 0;
}

static const luaL_Reg lib[] = {
	{"spawn", task_spawn},
	{"delay", task_delay},
	{"defer", task_defer},
	{"wait", task_wait},
	{"cancel", task_cancel},
	{nullptr, nullptr},
};

void task_lib_open(lua_State* L)
{
	luaL_register(L, "task", lib);
	lua_pop(L, 1);
}
