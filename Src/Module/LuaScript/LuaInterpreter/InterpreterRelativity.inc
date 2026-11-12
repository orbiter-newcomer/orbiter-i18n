// ==============================================================
// InterpreterRelativity.inc
// Lua bindings for the relativity extension.
// Included by Interpreter.cpp; keeps the implementation out of the official
// interpreter source file. (The static member declaration stays in
// Interpreter.h because the Lua function table references it.)
// ==============================================================

/***
Returns the relativistic time dilation of the focus vessel.
(The proper time rate d(tau)/dt, 1.0 = no time dilation.)
Also returns the gravitational and velocity factors separately.
@function get_time_dilation
@treturn number proper time rate d(tau)/dt (total)
@treturn number gravitational time dilation factor
@treturn number velocity Lorentz factor gamma
@see get_simtime, get_tacc
*/
int Interpreter::oapi_get_time_dilation (lua_State *L)
{
	lua_pushnumber (L, oapiGetRelativityTimeDilation());
	lua_pushnumber (L, oapiGetRelativityGravTimeFactor());
	lua_pushnumber (L, oapiGetRelativityVelTimeFactor());
	return 3;
}