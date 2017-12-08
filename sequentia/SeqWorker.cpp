#include "SeqWorker.h"
#include "SeqWorkerTask.h"
#include <SDL.h>

SeqWorker::SeqWorker():
	work(true),
	activeTask(nullptr),
	status(SeqWorkerStatus::Idle)
{
	mutex = SDL_CreateMutex();
	startTaskCond = SDL_CreateCond();
	SDL_CreateThread(ThreadProxy, "SeqWorker", this);
}

SeqWorker::~SeqWorker()
{
	SDL_DestroyCond(startTaskCond);
	SDL_DestroyMutex(mutex);
}

int SeqWorker::ThreadProxy(void *instance)
{
	((SeqWorker*)instance)->StartWork();
	return 0;
}

void SeqWorker::StartWork()
{
	while (work)
	{
		SDL_LockMutex(mutex);
		SDL_CondWait(startTaskCond, mutex);
		SDL_UnlockMutex(mutex);
		if (activeTask != nullptr)
		{
			activeTask->Start();
			status = SeqWorkerStatus::Finished;
		}
	}
	status = SeqWorkerStatus::Disposed;
}

void SeqWorker::StopWork()
{
	work = false;
	if (activeTask == nullptr)
	{
		StartTask(nullptr);
	}
	else
	{
		activeTask->Stop();
	}
}

void SeqWorker::StartTask(SeqWorkerTask *task)
{
	status = SeqWorkerStatus::Busy;
	activeTask = task;
	SDL_LockMutex(mutex);
	SDL_CondSignal(startTaskCond);
	SDL_UnlockMutex(mutex);
}

void SeqWorker::Finalize()
{
	activeTask->Finalize();
	activeTask = nullptr;
	status = SeqWorkerStatus::Idle;
}

SeqWorkerStatus SeqWorker::GetStatus()
{
	return status;
}
