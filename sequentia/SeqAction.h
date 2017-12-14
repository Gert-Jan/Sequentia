#pragma once

#include "stdint.h"

enum class SeqChannelType;
class SeqClip;
class SeqClipProxy;

enum class SeqActionType
{
	AddChannel,
	AddLibraryLink,
	AddClipToChannel,
	MoveClipToChannel,
	COUNT
};

enum class SeqActionExecution
{
	Do,
	Undo
};

struct SeqAction
{
	SeqActionType type;
	SeqActionExecution execution;
	void *data;
	SeqAction() { }
	SeqAction(SeqActionType type, SeqActionExecution execution, void *data): 
		type(type), execution(execution), data(data) { }
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

struct SeqActionAddClipToChannel
{
	int channelId;
	int libraryLinkIndex;
	int clipId;
	int64_t leftTime;
	int64_t rightTime;
	int64_t startTime;
	SeqActionAddClipToChannel(SeqClipProxy* proxy);
	SeqActionAddClipToChannel(SeqClip* clip);
};

struct SeqActionMoveClipToChannel
{
	int fromChannelId;
	int fromClipId;
	int toChannelId;
	int toClipId;
	SeqActionMoveClipToChannel(SeqClipProxy* proxy);
};
