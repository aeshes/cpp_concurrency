#include <iostream>
#include <string>
#include <functional>
#include <thread>
#include <chrono>
#include <queue>

#include <Windows.h>

#include "include\sqlite3.h"

#pragma comment(lib, ".\\lib\\sqlite3.lib")


struct func
{
	int& i;

	func(int& _i) : i(_i) {}

	void operator()()
	{
		Sleep(500);
		for (unsigned j = 0; j < 100000000; ++j)
			std::cout << (++i);
	}
};

class thread_guard
{
	std::thread& _thr;

public:
	explicit thread_guard(std::thread& th)
		: _thr(th) {}
	~thread_guard()
	{
		if (_thr.joinable())
			_thr.join();
	}

	thread_guard(thread_guard const&) = delete;
	thread_guard& operator=(thread_guard const&) = delete;
};

class db_connection
{
	static sqlite3 *db;
	db_connection()
	{
		int ret = sqlite3_open_v2(".\\data\\data.db", &db, SQLITE_OPEN_READONLY, nullptr);
		if (SQLITE_OK != ret)
		{
			std::cout << sqlite3_errmsg(db) << std::endl;
		}
	}
public:
	static sqlite3* getInstance()
	{
		static db_connection conn;
		return db;
	}

	db_connection(db_connection const&) = delete;
	db_connection& operator=(db_connection const&) = delete;
};

sqlite3* db_connection::db = nullptr;


int sqlite_callback(void* data, int argc, char** argv, char** colnames)
{
	for (int i = 0; i < argc; ++i)
	{
		std::cout << (argv[i] ? argv[i] : "NULL") << std::endl;
	}
	return 0;
}

void extract_data()
{
	sqlite3*    db = db_connection::getInstance();
	const char* sql = "SELECT * FROM employee WHERE id = 5";
	char*       errmsg = nullptr;

	int ret = sqlite3_exec(db, sql, sqlite_callback, nullptr, &errmsg);
}

struct background_task
{
	void operator()() const
	{
		extract_data();
	}
};

std::queue<background_task> task_queue;

static volatile bool stop_flag = false;

void run()
{
	while (!stop_flag)
	{
		if (!task_queue.empty())
		{
			task_queue.front()();
			task_queue.pop();
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
}


int main()
{
	std::thread daemon(run);
	daemon.detach();

	background_task task;
	task_queue.push(task);

	std::cout << "hw concurrency: " << std::thread::hardware_concurrency() << std::endl;
	
	int i;
	std::cin >> i;
	return 0;
}