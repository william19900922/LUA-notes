extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#include <stdio.h>
#include "luna.h"

class LuaTest
{
public:
	LuaTest()    {}
	LuaTest(lua_State* L)    {}

	static const char className[];
	static Luna<LuaTest>::RegType methods[];

public:
	int    TestString(lua_State* L)    { printf("hello!\n"); return 0; }
};

const char LuaTest::className[] = "LuaTest";
// Define the methods we will expose to Lua
#define method(class, name) {#name, &class::name}
Luna<LuaTest>::RegType LuaTest::methods[] =
{
	method(LuaTest, TestString),
	{ 0, 0 }
};

int main()
{
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	Luna<LuaTest>::Register(L);
	luaL_dofile(L, "Tlua.lua");

	lua_close(L);



	return 0;
}