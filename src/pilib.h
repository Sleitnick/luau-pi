#ifndef H_PILIB
#define H_PILIB

#include <lua.h>

void pilib_open(lua_State* L);
void pilib_call_exit_callbacks(lua_State* L);

#endif
