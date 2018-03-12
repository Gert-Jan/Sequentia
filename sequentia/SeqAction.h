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
	MoveClip,
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
	int sceneId;
	SeqChannelType type;
	char *name;
	SeqActionAddChannel(int sceneId, SeqChannelType type, const char *channelName);
	~SeqActionAddChannel() { delete[] name; }
};

struct SeqActionAddClipToChannel
{
	int sceneId;
	int channelId;
	int libraryLinkIndex;
	int streamIndex;
	int clipId;
	int64_t leftTime;
	int64_t rightTime;
	int64_t startTime;
	SeqActionAddClipToChannel(SeqClipProxy* proxy);
	SeqActionAddClipToChannel(SeqClip* clip);
};

struct SeqActionMoveClip
{
	int fromSceneId;
	int fromChannelId;
	int fromClipId;
	int64_t fromLeftTime;
	int toSceneId;
	int toChannelId;
	int toClipId;
	int64_t toLeftTime;
	SeqActionMoveClip(SeqClipProxy* proxy);
};
