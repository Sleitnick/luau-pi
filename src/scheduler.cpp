#include "scheduler.h"

#include <lualib.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <exception>

#include "threaddata.h"

static constexpr const char* kTaskScheduler = "TaskScheduler";
static constexpr int kMaxDeferEntryDepth = 40;

static std::string get_traceback(lua_State* L, int level)
{
	std::string s = "Begin Traceback\n";

	lua_Debug ar;
	for (int i = level; lua_getinfo(L, i, "sln", &ar); i++)
	{
		if (strcmp(ar.what, "C") == 0)
		{
			continue;
		}
		if (ar.source) {
			s += "  ";
			s += ar.short_src;
		}
		if (ar.currentline > 0)
		{
			char line[32]{};
			char* lineEnd = line + sizeof(line);
			char* linePtr = lineEnd;
			for (unsigned int r = ar.currentline; r > 0; r /= 10)
			{
				*--linePtr = '0' + (r % 10);
			}
			s += ':';
			s += std::string(linePtr, lineEnd - linePtr);
		}
		if (ar.name)
		{
			s += " - function ";
			s += ar.name;
		}
		s += '\n';
	}
	s += "End Traceback\n";
	return s;
}

LuauTaskScheduler* LuauTaskScheduler::create(lua_State* L)
{
	LuauTaskScheduler* scheduler = static_cast<LuauTaskScheduler*>(lua_newuserdata(L, sizeof(LuauTaskScheduler)));
	scheduler->state = L;
	scheduler->scheduled_tasks = new std::vector<ScheduledTask*>();
	scheduler->scheduled_tasks_temp = new std::vector<ScheduledTask*>();
	scheduler->deferred_tasks = new std::vector<ScheduledTask*>();
	scheduler->deferred_tasks_temp = new std::vector<ScheduledTask*>();
	scheduler->updating = false;
	scheduler->time = 0;
	lua_rawsetfield(L, LUA_REGISTRYINDEX, kTaskScheduler);

	return scheduler;
}

LuauTaskScheduler* LuauTaskScheduler::get(lua_State* L)
{
	lua_rawgetfield(L, LUA_REGISTRYINDEX, kTaskScheduler);
	LuauTaskScheduler* scheduler = static_cast<LuauTaskScheduler*>(lua_touserdata(L, -1));

	lua_pop(L, 1);

	return scheduler;
}

void LuauTaskScheduler::close()
{
	for (auto it = scheduled_tasks->begin(); it != scheduled_tasks->end(); ++it)
	{
		ScheduledTask* scheduled = *it;
		delete scheduled;
	}
	for (auto it = scheduled_tasks_temp->begin(); it != scheduled_tasks_temp->end(); ++it)
	{
		ScheduledTask* scheduled = *it;
		delete scheduled;
	}
	for (auto it = deferred_tasks->begin(); it != deferred_tasks->end(); ++it)
	{
		ScheduledTask* scheduled = *it;
		delete scheduled;
	}
	for (auto it = deferred_tasks_temp->begin(); it != deferred_tasks_temp->end(); ++it)
	{
		ScheduledTask* scheduled = *it;
		delete scheduled;
	}
	delete scheduled_tasks;
	delete scheduled_tasks_temp;
	delete deferred_tasks;
	delete deferred_tasks_temp;
}

lua_State* LuauTaskScheduler::create_thread(lua_State* L)
{
	lua_State* T = lua_newthread(L);
	luaL_sandboxthread(T);

	return T;
}

int LuauTaskScheduler::spawn(lua_State* T, lua_State* from, int n_args, bool can_yield)
{
	int status = lua_resume(T, from, n_args);

	if (status != LUA_OK && (status != LUA_YIELD || !can_yield))
	{
		// Handle error:
		if (status == LUA_YIELD)
		{
			printf("Thread cannot yield\n");
		}
		else
		{
			std::string err;

			if (const char* str = lua_tostring(T, -1))
			{
				err += str;
			}
			else
			{
				err += "Unknown error";
			}

			err += '\n';
			err += get_traceback(T, 1);

			printf("%s\n", err.c_str());
		}
	}

	return status;
}

void LuauTaskScheduler::delay(lua_State* T, lua_State* from, int n_args, int thread_ref, double delay_time, bool yield_delta)
{
	(void)T;

	ScheduledTask* scheduled = new ScheduledTask();
	scheduled->n_args = n_args;
	scheduled->resume_at = time + delay_time;
	scheduled->thread_ref = thread_ref;
	scheduled->erase = false;
	scheduled->start = time;
	scheduled->yield_delta = yield_delta;

	lua_pushthread(from);
	scheduled->from_thread_ref = lua_ref(from, -1);
	lua_pop(from, 1);

	auto tasks = updating ? scheduled_tasks_temp : scheduled_tasks;
	tasks->push_back(scheduled);
}

bool LuauTaskScheduler::defer(lua_State* T, lua_State* from, int n_args, int thread_ref)
{
	(void)T;

	size_t defer_depth = 1;
	if (from)
	{
		ThreadData* parent_td = static_cast<ThreadData*>(lua_getthreaddata(from));
		if (parent_td)
		{
			defer_depth += parent_td->defer_depth;
		}
	}

	if (defer_depth > kMaxDeferEntryDepth)
	{
		return false;
	}

	ScheduledTask* scheduled = new ScheduledTask();
	scheduled->n_args = n_args;
	scheduled->resume_at = 0;
	scheduled->thread_ref = thread_ref;
	scheduled->erase = false;

	lua_pushthread(from);
	scheduled->from_thread_ref = lua_ref(from, -1);
	lua_pop(from, 1);

	auto tasks = updating ? deferred_tasks_temp : deferred_tasks;
	tasks->push_back(scheduled);

	return true;
}

void LuauTaskScheduler::cancel(lua_State* T)
{
	lua_resetthread(T);

	auto it = scheduled_tasks->begin();
	while (it != scheduled_tasks->end())
	{
		ScheduledTask* scheduled = *it;

		lua_getref(state, scheduled->thread_ref);
		lua_State* thread = lua_tothread(state, -1);
		lua_pop(state, 1);

		lua_getref(state, scheduled->from_thread_ref);
		lua_State* from = lua_tothread(state, -1);
		lua_pop(state, 1);

		if (T == thread || T == from)
		{
			if (updating)
			{
				scheduled->erase = true;
				++it;
			}
			else
			{
				it = scheduled_tasks->erase(it);
				delete scheduled;
			}
		}
		else
		{
			++it;
		}
	}

	auto it_deferred = deferred_tasks->begin();
	while (it_deferred != deferred_tasks->end())
	{
		ScheduledTask* scheduled = *it_deferred;

		lua_getref(state, scheduled->thread_ref);
		lua_State* thread = lua_tothread(state, -1);
		lua_pop(state, 1);

		lua_getref(state, scheduled->from_thread_ref);
		lua_State* from = lua_tothread(state, -1);
		lua_pop(state, 1);

		if (T == thread || T == from)
		{
			if (updating)
			{
				scheduled->erase = true;
				++it_deferred;
			}
			else
			{
				it_deferred = deferred_tasks->erase(it_deferred);
				delete scheduled;
			}
		}
		else
		{
			++it_deferred;
		}
	}
}

bool LuauTaskScheduler::update(double now, double dt)
{
	updating = true;
	time = now;

	// Run scheduled tasks:
	auto it = scheduled_tasks->begin();
	while (it != scheduled_tasks->end())
	{
		ScheduledTask* scheduled = *it;
		if (now >= scheduled->resume_at)
		{
			lua_getref(state, scheduled->thread_ref);
			lua_State* T = lua_tothread(state, -1);
			lua_pop(state, 1);

			lua_getref(state, scheduled->from_thread_ref);
			lua_State* from = lua_tothread(state, -1);
			lua_pop(state, 1);

			if (scheduled->yield_delta && scheduled->n_args == 0)
			{
				double delta = now - scheduled->start;
				lua_pushnumber(T, delta);
				spawn(T, T == from ? nullptr : from, 1);
			}
			else
			{
				spawn(T, T == from ? nullptr : from, scheduled->n_args);
			}

			lua_unref(state, scheduled->from_thread_ref);
			it = scheduled_tasks->erase(it);
			delete scheduled;
		}
		else if (scheduled->erase)
		{
			it = scheduled_tasks->erase(it);
			delete scheduled;
		}
		else
		{
			++it;
		}
	}

	// Append the swapped scheduled tasks to the actual scheduled tasks vector:
	if (scheduled_tasks_temp->size() > 0)
	{
		scheduled_tasks->insert(scheduled_tasks->end(), scheduled_tasks_temp->begin(), scheduled_tasks_temp->end());
		scheduled_tasks_temp->clear();
	}

	// Run deferred tasks:
	auto it2 = deferred_tasks->begin();
	while (it2 != deferred_tasks->end())
	{
		ScheduledTask* scheduled = *it2;

		lua_getref(state, scheduled->thread_ref);
		lua_State* T = lua_tothread(state, -1);
		lua_pop(state, 1);

		lua_getref(state, scheduled->from_thread_ref);
		lua_State* from = lua_tothread(state, -1);
		lua_pop(state, 1);

		spawn(T, T == from ? nullptr : from, scheduled->n_args);

		// Reset defer depth:
		ThreadData* td = static_cast<ThreadData*>(lua_getthreaddata(T));
		td->defer_depth = 0;

		lua_unref(state, scheduled->from_thread_ref);
		delete scheduled;

		if (deferred_tasks_temp->size() > 0)
		{
			deferred_tasks->insert(deferred_tasks->end(), deferred_tasks_temp->begin(), deferred_tasks_temp->end());
			deferred_tasks_temp->clear();

			// Inserting invalidates all iterators, so we need to find the iterator again:
			it2 = std::find(deferred_tasks->begin(), deferred_tasks->end(), scheduled);
			if (it2 == deferred_tasks->end())
			{
				break;
			}
		}

		++it2;
	}
	deferred_tasks->clear();
	
	updating = false;

	return !scheduled_tasks->empty();
}
