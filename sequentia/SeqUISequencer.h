#pragma once

class SeqProject;
template<class T>
class SeqList;

class SeqUISequencer : SeqActionHandler
{
public:
	SeqUISequencer(SeqProject *project, int windowId);
	SeqUISequencer(SeqProject *project);
	~SeqUISequencer();

	void ActionDone(const SeqAction action);
	void ActionUndone(const SeqAction action);
	void Draw();

private:
	void Init();
	void DrawChannelSettings(float rulerHeight, bool isWindowNew);
	void DrawSequencerRuler(float height);
	void DrawChannels();
	int TotalChannelHeight();

private:
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
	static int nextWindowId;
	int windowId = 0;
	char *name;
	SeqProject *project;
	SeqList<int> *channelHeights;
	double position;
	float zoom = 1;
	float scrollY = 0;
	float settingsPanelWidth = 100;
	bool isSettingsPanelCollapsed = false;
};
