#pragma once

class SeqWorkerTask;
struct SDL_mutex;
struct SDL_cond;

enum class SeqWorkerStatus
{
	Idle,
	Busy,
	Finished,
	Disposed
};

class SeqWorker
{
public:
	SeqWorker();
	~SeqWorker();

	// thread entry
	void StartWork();
	// gracefully stops the current task and stops the thread, eventually set's the status to Disposed
	void StopWork();
	// sets a task on the worker whom will start executing it in it's thread asap
	void StartTask(SeqWorkerTask *task);
	// called from the main thread in order to safely integrate task results
	void Finalize();
	// worker status
	SeqWorkerStatus GetStatus();
private:
	// static, SDL compatible thread reference
	static int ThreadProxy(void* instance);

private:
	bool work;
	SDL_mutex *mutex;
	SDL_cond *startTaskCond;
	SeqWorkerStatus status;
	SeqWorkerTask *activeTask;
};
