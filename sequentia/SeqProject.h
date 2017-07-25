#pragma once

#include "SeqChannel.h";

template<class T>
class SeqList;

class SeqProject
{
public:
	SeqProject();
	~SeqProject();

	void AddChannel(SeqChannelType type, char *name);
	void RemoveChannel(const int index);

private:
	SeqList<SeqChannel> *channels;
};
