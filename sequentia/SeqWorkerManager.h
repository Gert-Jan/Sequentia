#pragma once

class SeqWorker;
class SeqWorkerTask;

template<class T>
class SeqList;

class SeqWorkerManager
{
public:
	static SeqWorkerManager* Instance();
	void PerformTask(SeqWorkerTask *task);
	void Update();
	void DrawDebugWindow();
private:
	SeqWorkerManager();
	~SeqWorkerManager();
	void FinalizeTasks();
	void StartTasks();
private:
	const int initialWorkerCount = 8;
	SeqList<SeqWorker*> *workers;
	SeqList<SeqWorkerTask*> *taskQueue;
};
