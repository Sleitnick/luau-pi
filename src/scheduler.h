#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <lua.h>
#include <vector>

struct ScheduledTask
{
	int thread_ref;
	int from_thread_ref;
	int n_args;
	double resume_at;
	double start;
	bool yield_delta;
	bool erase;
};

class LuauTaskScheduler
{
private:
	lua_State* state;
	std::vector<ScheduledTask*>* scheduled_tasks;
	std::vector<ScheduledTask*>* scheduled_tasks_temp;
	std::vector<ScheduledTask*>* deferred_tasks;
	std::vector<ScheduledTask*>* deferred_tasks_temp;

	bool updating;
	int defer_entry_depth;
	double time;

public:
	static LuauTaskScheduler* create(lua_State* L);
	static LuauTaskScheduler* get(lua_State* L);

	lua_State* create_thread(lua_State* L);
	int spawn(lua_State* T, lua_State* from, int n_args, bool can_yield = true);
	bool defer(lua_State* T, lua_State* from, int n_args, int thread_ref);
	void delay(lua_State* T, lua_State* from, int n_args, int thread_ref, double delay_time, bool yield_delta);
	void cancel(lua_State* T);

	bool update(double now, double dt);
	void close();
};

#endif
