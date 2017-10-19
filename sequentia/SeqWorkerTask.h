#pragma once

enum SeqWorkerTaskPriority
{
	Low, Medium, High
};

class SeqWorkerTask
{
public:
	// worker thread: do the work
	virtual void Start() = 0;
	// main thread: stop a continues task
	virtual void Stop() = 0;
	// main thread: designed to copy task results back to the data model
	virtual void Finalize() = 0;
	// main thread: used by the SeqWorkerManager to prioritize tasks
	virtual SeqWorkerTaskPriority GetPriority() = 0;
	
	//debug
	// main thread: returns a number between 0 and 1 indicating how far the task has progressed
	virtual float GetProgress() = 0;
};
