#ifndef _luna_h_
#define _luna_h_
/**
* Taken directly from http://lua-users.org/wiki
*/

/* Lua */
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

template <typename T> class Luna {
	typedef struct { T *pT; } userdataType;
public:
	typedef int (T::*mfp)(lua_State *L);
	typedef struct { const char *name; mfp mfunc; } RegType;

	static void Register(lua_State *L) {
		//新建一个table,methods保存其栈索引,这个方法表就是用来保存要导出的成员函数
		lua_newtable(L);
		int methods = lua_gettop(L);
		//在注册表中新建一个元表,metatable保存其栈索引.
		//元表是模拟面向对象机制的关键.后面将会把该元表赋予fulluserdata(C++对象在lua中的映射对象).
		luaL_newmetatable(L, T::className);
		int metatable = lua_gettop(L);

		//全局表[T::className]=methods表
		lua_pushstring(L, T::className);
		lua_pushvalue(L, methods);
		lua_settable(L, LUA_GLOBALSINDEX);

		//设置metatable元表的__metatable元事件
		//作用是将元表封装起来,防止外部的获取和修改
		lua_pushliteral(L, "__metatable");
		lua_pushvalue(L, methods);
		lua_settable(L, metatable);  // hide metatable from Lua getmetatable()

		//设置metatable元表的__index元事件指向methods表
		//该事件会在元表onwer被索引不存在成员时触发,这时就会去methods表中进行索引...
		lua_pushliteral(L, "__index");
		lua_pushvalue(L, methods);
		lua_settable(L, metatable);

		//设置元表的__tostring和__gc元事件
		//前者是为了支持print(MyObj)这样的用法..
		//后者是设置我们的lua对象被垃圾回收时的一个回调.
		lua_pushliteral(L, "__tostring");
		lua_pushcfunction(L, tostring_T);
		lua_settable(L, metatable);

		lua_pushliteral(L, "__gc");
		lua_pushcfunction(L, gc_T);
		lua_settable(L, metatable);

		//下面一段代码干了这么些事:
		//1.创建方法表的元表mt
		//2.方法表.new = new_T
		//3.设置mt的__call元事件.该事件会在lua执行到a()这样的函数调用形式时触发.
		//这使得我们可以重写该事件使得能对table进行调用...如t()
		lua_newtable(L);                // mt for method table
		int mt = lua_gettop(L);
		lua_pushliteral(L, "__call");
		lua_pushcfunction(L, new_T);
		lua_pushliteral(L, "new");
		lua_pushvalue(L, -2);           // dup new_T function
		lua_settable(L, methods);       // add new_T to method table
		lua_settable(L, mt);            // mt.__call = new_T
		lua_setmetatable(L, methods);

		// fill method table with methods from class T
		for (RegType *l = T::methods; l->name; l++) {
			lua_pushstring(L, l->name);
			lua_pushlightuserdata(L, (void*)l);
			lua_pushcclosure(L, thunk, 1);        //创建闭包,附带数据为RegType项
			lua_settable(L, methods);                //方法表[l->name]=闭包
		}

		//平衡栈
		lua_pop(L, 2);  // drop metatable and method table
	}

	// get userdata from Lua stack and return pointer to T object
	static T *check(lua_State *L, int narg) {
		userdataType *ud =
			static_cast<userdataType*>(luaL_checkudata(L, narg, T::className));
		if (!ud) luaL_typerror(L, narg, T::className);
		return ud->pT;  // pointer to T object
	}

private:
	Luna();  // hide default constructor

	// member function dispatcher
	static int thunk(lua_State *L) {
		// stack has userdata, followed by method args
		T *obj = check(L, 1);  // get 'self', or if you prefer, 'this'
		lua_remove(L, 1);  // remove self so member function args start at index 1
		// get member function from upvalue
		RegType *l = static_cast<RegType*>(lua_touserdata(L, lua_upvalueindex(1)));
		return (obj->*(l->mfunc))(L);  // call member function
	}

	// create a new T object and
	// push onto the Lua stack a userdata containing a pointer to T object
	static int new_T(lua_State *L) {
		lua_remove(L, 1);   // use classname:new(), instead of classname.new()
		T *obj = new T(L);  // call constructor for T objects
		userdataType *ud =
			static_cast<userdataType*>(lua_newuserdata(L, sizeof(userdataType)));
		ud->pT = obj;  // store pointer to object in userdata
		luaL_getmetatable(L, T::className);  // lookup metatable in Lua registry
		lua_setmetatable(L, -2);
		return 1;  // userdata containing pointer to T object
	}

	// garbage collection metamethod
	static int gc_T(lua_State *L) {
		userdataType *ud = static_cast<userdataType*>(lua_touserdata(L, 1));
		T *obj = ud->pT;
		delete obj;  // call destructor for T objects
		return 0;
	}

	static int tostring_T(lua_State *L) {
		char buff[32];
		userdataType *ud = static_cast<userdataType*>(lua_touserdata(L, 1));
		T *obj = ud->pT;
		sprintf_s(buff, "%p", obj);
		lua_pushfstring(L, "%s (%s)", T::className, buff);
		return 1;
	}
};
#endif