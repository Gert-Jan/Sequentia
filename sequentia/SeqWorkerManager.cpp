#include "SeqWorkerManager.h"
#include "SeqWorker.h"
#include "SeqWorkerTask.h"
#include "SeqList.h"

SeqWorkerManager* SeqWorkerManager::Instance()
{
	static SeqWorkerManager instance;
	return &instance;
}

SeqWorkerManager::SeqWorkerManager()
{
	workers = new SeqList<SeqWorker*>();
	for (int i = 0; i < initialWorkerCount; i++)
		workers->Add(new SeqWorker());
	taskQueue = new SeqList<SeqWorkerTask*>();
}

SeqWorkerManager::~SeqWorkerManager()
{
	delete taskQueue;
	for (int i = 0; i < workers->Count(); i++)
		delete workers->Get(i);
	delete workers;
}

void SeqWorkerManager::PerformTask(SeqWorkerTask *task)
{
	taskQueue->Add(task);
	StartTasks();
}

void SeqWorkerManager::Update()
{
	FinalizeTasks();
	StartTasks();
}

void SeqWorkerManager::FinalizeTasks()
{
	for (int i = 0; i < workers->Count(); i++)
	{
		if (workers->Get(i)->GetStatus() == SeqWorkerStatus::Finished)
			workers->Get(i)->Finalize();
	}
}

void SeqWorkerManager::StartTasks()
{
	for (int i = 0; i < workers->Count() && taskQueue->Count() > 0; i++)
	{
		if (workers->Get(i)->GetStatus() == SeqWorkerStatus::Idle)
		{
			workers->Get(i)->StartTask(taskQueue->Get(0));
			taskQueue->RemoveAt(0);
		}
	}
}
