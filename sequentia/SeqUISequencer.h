#pragma once

#include "SeqWindow.h"
#include "SeqAction.h"

class SeqScene;
class SeqChannel;
class SeqClip;
class SeqSerializer;
template<class T>
class SeqList;

struct SeqSceneUISettings
{
	SeqScene *scene;
	SeqList<int> *channelHeights;
};

class SeqUISequencer : public SeqWindow, SeqActionHandler
{
public:
	SeqUISequencer(SeqScene *scene);
	SeqUISequencer(SeqScene *scene, SeqSerializer *serializer);
	~SeqUISequencer();

	void PreExecuteAction(const SeqActionType type, const SeqActionExecution execution, const void *data);
	SeqWindowType GetWindowType();
	void Draw();
	static void DrawClip(SeqClip *clip, const ImVec2 position, const ImVec2 size, const bool isHovered = false);
	void Serialize(SeqSerializer *serializer);

private:
	void Init();
	void DrawChannelSettings(float rulerHeight, bool isWindowNew);
	void DrawSequencerRuler(float height);
	void DrawChannels();
	void DrawChannel(SeqChannel *channel, ImVec2 cursor, ImVec2 availableSize, ImVec2 contentSize, float height);
	bool ClipInteraction(SeqClip *clip, const ImVec2 position, const ImVec2 size, bool *isHovered);
	SeqSceneUISettings* GetSceneUISettings(SeqScene *scene);
	int TotalChannelHeight();
	int64_t PixelsToTime(float pixels);
	float TimeToPixels(int64_t time);
	void Deserialize(SeqSerializer *serializer);

private:
	static ImU32 lineColor;
	static ImU32 backgroundColor;
	static ImU32 clipBackgroundColor;
	const float pixelsPerSecond = 100;
	const float minSettingsPanelWidth = 40;
	const float maxSettingsPanelWidth = 300;
	const float minChannelHeight = 20;
	const float maxChannelHeight = 300;
	const int initialChannelHeight = 50;
	const int channelVerticalSpacing = -1;
	const float collapsedSettingsPanelWidth = 10;
	const float lineThickness = 1.0f;
	const float rounding = 6.0f;
	char *name;
	SeqScene *scene;
	SeqSceneUISettings *sceneSettings;
	SeqList<SeqSceneUISettings> *sceneUISettings;
	int64_t position;
	float zoom = 1;
	bool overrideScrollX = true;
	ImVec2 scroll = ImVec2(0, 0);
	int64_t dragStartPosition = 0;
	float settingsPanelWidth = 100;
	bool isSettingsPanelCollapsed = false;
};
