//#include "SpecialList.h";
#include "SeqList.h";
#include "SeqProject.h";

SeqProject::SeqProject()
{
	channels = new SeqList<SeqChannel>(2);
}

SeqProject::~SeqProject()
{
	delete channels;
}

void SeqProject::AddChannel(SeqChannelType type, char *name)
{
	SeqChannel channel = SeqChannel(name, type);
	channels->Add(channel);
}

void SeqProject::RemoveChannel(const int index)
{
	channels->RemoveAt(index);
}