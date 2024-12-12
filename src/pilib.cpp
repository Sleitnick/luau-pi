#include "pilib.h"

#include <lualib.h>
#include <wiringPi.h>
#include <cstdio>

#include "scheduler.h"

constexpr const char* k_on_exit_callbacks = "OnExitCallbacks";

#define PUSH_ENUM(L, name) lua_pushinteger((L), (name)); lua_rawsetfield((L), -2, #name)

static int pi_wiringPiGpioDeviceGetFd(lua_State* L)
{
	lua_pushinteger(L, wiringPiGpioDeviceGetFd());
	return 1;
}

static int pi_pinMode(lua_State* L)
{
	int pin = luaL_checkinteger(L, 1);
	int mode = luaL_checkinteger(L, 2);

	pinMode(pin, mode);

	return 0;
}

static int pi_pullUpDnControl(lua_State* L)
{
	int pin = luaL_checkinteger(L, 1);
	int pud = luaL_checkinteger(L, 2);

	pullUpDnControl(pin, pud);

	return 0;
}

static int pi_digitalRead(lua_State* L)
{
	int pin = luaL_checkinteger(L, 1);

	lua_pushboolean(L, digitalRead(pin));

	return 1;
}

static int pi_digitalWrite(lua_State* L)
{
	int pin = luaL_checkinteger(L, 1);
	luaL_argcheck(L, lua_isboolean(L, 2) || lua_isnumber(L, 2), 2, "expected boolean or number");
	
	int state;
	if (lua_isboolean(L, 2))
	{
		state = lua_toboolean(L, 2);
	}
	else
	{
		state = lua_tointeger(L, 2) != 0;
	}

	digitalWrite(pin, state);

	return 0;
}

static int pi_pwmWrite(lua_State* L)
{
	int pin = luaL_checkinteger(L, 1);
	int value = luaL_checkinteger(L, 2);

	pwmWrite(pin, value);

	return 0;
}

static int pi_analogRead(lua_State* L)
{
	int pin = luaL_checkinteger(L, 1);

	lua_pushinteger(L, analogRead(pin));

	return 1;
}

static int pi_analogWrite(lua_State* L)
{
	int pin = luaL_checkinteger(L, 1);
	int value = luaL_checkinteger(L, 2);

	analogWrite(pin, value);

	return 0;
}

static int pi_setup(lua_State* L)
{
	lua_pushboolean(L, wiringPiSetup() != -1);
	return 1;
}

static int pi_setupSys(lua_State* L)
{
	lua_pushboolean(L, wiringPiSetupSys() != -1);
	return 1;
}

static int pi_setupGpio(lua_State* L)
{
	lua_pushboolean(L, wiringPiSetupGpio() != -1);
	return 1;
}

static int pi_setupPhys(lua_State* L)
{
	lua_pushboolean(L, wiringPiSetupPhys() != -1);
	return 1;
}

static int pi_onExit(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TFUNCTION);

	lua_rawgetfield(L, LUA_REGISTRYINDEX, k_on_exit_callbacks);

	lua_pushvalue(L, 1); // fn
	lua_rawseti(L, -2, lua_objlen(L, -2) + 1); // callbacks[#callbacks + 1] = fn

	lua_pop(L, 1);

	return 0;
}

static const luaL_Reg pi_lib[] = {
	{"wiringPiGpioDeviceGetFd", pi_wiringPiGpioDeviceGetFd},
	{"pinMode", pi_pinMode},
	{"pullUpDownControl", pi_pullUpDnControl},
	{"digitalRead", pi_digitalRead},
	{"digitalWrite", pi_digitalWrite},
	{"pwmWrite", pi_pwmWrite},
	{"analogRead", pi_analogRead},
	{"analogWrite", pi_analogWrite},
	{"setup", pi_setup},
	{"setupSys", pi_setupSys},
	{"setupGpio", pi_setupGpio},
	{"setupPhys", pi_setupPhys},
	{"onExit", pi_onExit},
	{nullptr, nullptr},
};

void pilib_call_exit_callbacks(lua_State* L)
{
	lua_rawgetfield(L, LUA_REGISTRYINDEX, k_on_exit_callbacks);
	int n = lua_objlen(L, -1);

	LuauTaskScheduler* scheduler = LuauTaskScheduler::get(L);

	for (int i = 1; i <= n; i++)
	{
		lua_rawgeti(L, -1, i);
		lua_State* T = scheduler->create_thread(L);
		lua_insert(L, -2);
		lua_xmove(L, T, 1);
		(void)scheduler->spawn(T, L, 0, true);
		lua_pop(L, 1);
	}

	lua_pop(L, 1);
}

void pilib_open(lua_State* L)
{
	luaL_register(L, "pi", pi_lib);

	PUSH_ENUM(L, INPUT);
	PUSH_ENUM(L, OUTPUT);
	PUSH_ENUM(L, PWM_OUTPUT);
	PUSH_ENUM(L, PWM_MS_OUTPUT);
	PUSH_ENUM(L, PWM_BAL_OUTPUT);
	PUSH_ENUM(L, GPIO_CLOCK);
	PUSH_ENUM(L, SOFT_PWM_OUTPUT);
	PUSH_ENUM(L, SOFT_TONE_OUTPUT);
	PUSH_ENUM(L, PWM_TONE_OUTPUT);
	PUSH_ENUM(L, PM_OFF);

	PUSH_ENUM(L, LOW);
	PUSH_ENUM(L, HIGH);

	PUSH_ENUM(L, PUD_OFF);
	PUSH_ENUM(L, PUD_DOWN);
	PUSH_ENUM(L, PUD_UP);

	PUSH_ENUM(L, PWM_MODE_MS);
	PUSH_ENUM(L, PWM_MODE_BAL);

	PUSH_ENUM(L, INT_EDGE_SETUP);
	PUSH_ENUM(L, INT_EDGE_FALLING);
	PUSH_ENUM(L, INT_EDGE_RISING);
	PUSH_ENUM(L, INT_EDGE_BOTH);

	PUSH_ENUM(L, PI_MODEL_A);
	PUSH_ENUM(L, PI_MODEL_B);
	PUSH_ENUM(L, PI_MODEL_AP);
	PUSH_ENUM(L, PI_MODEL_BP);
	PUSH_ENUM(L, PI_MODEL_2);
	PUSH_ENUM(L, PI_ALPHA);
	PUSH_ENUM(L, PI_MODEL_CM);
	PUSH_ENUM(L, PI_MODEL_07);
	PUSH_ENUM(L, PI_MODEL_3B);
	PUSH_ENUM(L, PI_MODEL_ZERO);
	PUSH_ENUM(L, PI_MODEL_CM3);
	PUSH_ENUM(L, PI_MODEL_ZERO_W);
	PUSH_ENUM(L, PI_MODEL_3BP);
	PUSH_ENUM(L, PI_MODEL_3AP);
	PUSH_ENUM(L, PI_MODEL_CM3P);
	PUSH_ENUM(L, PI_MODEL_4B);
	PUSH_ENUM(L, PI_MODEL_ZERO_2W);
	PUSH_ENUM(L, PI_MODEL_400);
	PUSH_ENUM(L, PI_MODEL_CM4);
	PUSH_ENUM(L, PI_MODEL_CM4S);
	PUSH_ENUM(L, PI_MODEL_5);

	PUSH_ENUM(L, PI_VERSION_1);
	PUSH_ENUM(L, PI_VERSION_1_1);
	PUSH_ENUM(L, PI_VERSION_1_2);
	PUSH_ENUM(L, PI_VERSION_2);

	PUSH_ENUM(L, PI_MAKER_SONY);
	PUSH_ENUM(L, PI_MAKER_EGOMAN);
	PUSH_ENUM(L, PI_MAKER_EMBEST);
	PUSH_ENUM(L, PI_MAKER_UNKNOWN);

	PUSH_ENUM(L, GPIO_LAYOUT_PI1_REV1);
	PUSH_ENUM(L, GPIO_LAYOUT_DEFAULT);

	lua_pop(L, 1);

	// OnExitCallbacks table:
	lua_newtable(L);
	lua_rawsetfield(L, LUA_REGISTRYINDEX, k_on_exit_callbacks);
}
