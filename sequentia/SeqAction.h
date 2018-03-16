#pragma once

#include "SDL_config.h"

class SeqScene;
enum class SeqChannelType;
class SeqClip;
class SeqSelection;
class SeqClipGroup;

enum class SeqActionType
{
	AddChannel,
	AddLibraryLink,
	AddClipGroup,
	AddClipToChannel,
	AddClipToGroup,
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
	virtual void PreExecuteAction(const SeqActionType type, const SeqActionExecution execution, const void *data) = 0;
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
	SeqActionAddClipToChannel(SeqSelection* proxy);
	SeqActionAddClipToChannel(SeqClip* clip);
};

struct SeqActionAddClipGroup
{
	int sceneId;
	int groupId;
	SeqActionAddClipGroup(SeqScene* scene);
};

struct SeqActionAddClipToGroup
{
	int sceneId;
	int channelId;
	int clipId;
	int groupId;
	SeqActionAddClipToGroup(SeqClip* clip, SeqClipGroup* group);
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
	SeqActionMoveClip(SeqSelection* proxy);
};
