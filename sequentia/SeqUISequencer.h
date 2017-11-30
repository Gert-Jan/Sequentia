#pragma once

#include "SeqWindow.h";
#include "SeqAction.h";

class SeqProject;
class SeqChannel;
class SeqSerializer;
template<class T>
class SeqList;

class SeqUISequencer : public SeqWindow, SeqActionHandler
{
public:
	SeqUISequencer(SeqProject *project);
	SeqUISequencer(SeqProject *project, SeqSerializer *serializer);
	~SeqUISequencer();

	void ActionDone(const SeqAction action);
	void ActionUndone(const SeqAction action);
	SeqWindowType GetWindowType();
	void Draw();
	void Serialize(SeqSerializer *serializer);

private:
	void Init();
	void DrawChannelSettings(float rulerHeight, bool isWindowNew);
	void DrawSequencerRuler(float height);
	void DrawChannels();
	void DrawChannel(SeqChannel *channel, ImVec2 cursor, ImVec2 contentSize, float height);
	int TotalChannelHeight();
	double PixelsToTime(float pixels);
	float TimeToPixels(double time);
	void Deserialize(SeqSerializer *serializer);

private:
	ImU32 lineColor;
	ImU32 backgroundColor;
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
	SeqProject *project;
	SeqList<int> *channelHeights;
	double position;
	float zoom = 1;
	bool overrideScrollX = true;
	ImVec2 scroll = ImVec2(0, 0);
	double dragStartPosition = 0;
	float settingsPanelWidth = 100;
	bool isSettingsPanelCollapsed = false;
};
