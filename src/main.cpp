#include <string>
#include <cstring>
#include <wiringPi.h>
#include <lua.h>
#include <lualib.h>
#include <luacode.h>
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <thread>
#include <chrono>

#include "state.h"
#include "script.h"
#include "scheduler.h"
#include "pilib.h"
#include "fs.h"

#define VERSION "luau-pi v0.1.0"

using namespace LuauPi;

volatile bool stop_script = false;

static void handle_sigint(int s)
{
	stop_script = true;
}

static void print_version()
{
	printf("%s\n", VERSION);
}

static void print_help()
{
	print_version();
	printf("\n");
	printf("Usage: luaupi [COMMAND]\n\n");
	printf("Commands:\n");
	printf("   run [FILE]\n");
	printf("   version\n");
	printf("   help\n");
	printf("\n");
}

static int run_script(const char* filepath)
{
	struct sigaction sigint_handler{};
	sigint_handler.sa_handler = handle_sigint;
	sigemptyset(&sigint_handler.sa_mask);
	sigint_handler.sa_flags = 0;

	sigaction(SIGINT, &sigint_handler, nullptr);

	LuauState state;
	lua_State* L = state.get();

	LuauTaskScheduler* scheduler = LuauTaskScheduler::get(L);

	if (LuauScript::load_and_run(L, filepath, nullptr) == nullptr)
	{
		return 1;
	}

	double last = lua_clock();
	while (!stop_script)
	{
		double now = lua_clock();
		double dt = now - last;
		last = now;

		bool has_more = scheduler->update(now, dt);
		if (!has_more)
		{
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	pilib_call_exit_callbacks(L);

	return stop_script ? 1 : 0;
}

int main(int argc, char** argv)
{
	if (argc <= 1)
	{
		printf("No command provided\n\n");
		print_help();
		return 1;
	}

	if (strcmp(argv[1], "help") == 0)
	{
		print_help();
		return 0;
	}
	else if (strcmp(argv[1], "version") == 0)
	{
		print_version();
		return 0;
	}
	else if (strcmp(argv[1], "run") == 0)
	{
		if (argc <= 2)
		{
			printf("No file provided\n");
			return 1;
		}
		return run_script(argv[2]);
	}

	printf("Unknown command: %s\n", argv[1]);
	print_help();

	return 1;
}
