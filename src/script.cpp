#include "script.h"

#include <string>
#include <luacode.h>
#include <lualib.h>
#include <cstdio>
#include <memory>

#include "fs.h"
#include "scheduler.h"

using namespace LuauPi;

lua_State* LuauScript::load_and_run(lua_State* L, const std::string& filepath, int* status)
{
	if (status != nullptr)
	{
		*status = -1;
	}

	std::string source = FS::read_file(filepath);

	const char* mutable_globals[] = { nullptr };
	const char* userdata_types[] = { nullptr };

	lua_CompileOptions compile_options{};
	compile_options.optimizationLevel = 1;
	compile_options.debugLevel = 1;
	compile_options.typeInfoLevel = 0;
	compile_options.coverageLevel = 0;
	compile_options.mutableGlobals = mutable_globals;
	compile_options.userdataTypes = userdata_types;

	size_t bytecode_size;
	std::unique_ptr<char, void(*)(void*)> bytecode_ptr = std::unique_ptr<char, void(*)(void*)>(luau_compile(source.data(), source.size(), &compile_options, &bytecode_size), free);

	lua_pushthread(L);
	int l_pin = lua_ref(L, -1);
	lua_pop(L, 1);

	int result = luau_load(L, (std::string("=") + filepath).c_str(), bytecode_ptr.get(), bytecode_size, 0);

	if (result != LUA_OK)
	{
		size_t len;
		const char* msg = lua_tolstring(L, -1, &len);
		lua_pop(L, 1);
		lua_unref(L, l_pin);
		printf("[ERROR] %s\n", msg);
		return nullptr;
	}

	LuauTaskScheduler* scheduler = LuauTaskScheduler::get(L);

	lua_State* T = scheduler->create_thread(L);
	lua_pop(L, 1);

	lua_pushvalue(L, -1);
	lua_remove(L, -2);
	lua_xmove(L, T, 1);

	int spawn_status = scheduler->spawn(T, nullptr, 0);
	if (status != nullptr)
	{
		*status = spawn_status;
	}

	lua_unref(L, l_pin);

	return T;
}
