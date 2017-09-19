#pragma once

enum SeqChannelType;

enum SeqActionType
{
	AddChannel,
	RemoveChannel,
	AddLibraryLink,
	RemoveLibraryLink,
	COUNT
};

struct SeqAction
{
	SeqActionType type;
	void *data;
	SeqAction() { }
	SeqAction(SeqActionType type, void *data): type(type), data(data) { }
};

class SeqActionHandler
{
public:
	virtual void ActionDone(const SeqAction action) = 0;
	virtual void ActionUndone(const SeqAction action) = 0;
};

struct SeqActionAddChannel
{
	SeqChannelType type;
	char *name;
	SeqActionAddChannel(SeqChannelType type, char *name): type(type), name(name) { }
	~SeqActionAddChannel() { delete[] name; }
};
