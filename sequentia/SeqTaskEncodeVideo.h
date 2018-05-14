#pragma once

#include "SeqWorkerTask.h"

enum class SeqWorkerTaskPriority;
class SeqEncoder;
struct SDL_mutex;

class SeqTaskEncodeVideo : public SeqWorkerTask
{
public:
	SeqTaskEncodeVideo();
	~SeqTaskEncodeVideo();
	void Start();
	void Stop();
	void Finalize();
	SeqEncoder* GetEncoder();
	SeqWorkerTaskPriority GetPriority();
	float GetProgress();
	char* GetName();
private:
	SeqEncoder *encoder;
	int error;
	bool done;
};
