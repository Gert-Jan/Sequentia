#pragma once

class SeqProject;
template<class T>
class SeqList;

class SeqUISequencer : SeqActionHandler
{
public:
	SeqUISequencer(SeqProject *project);
	~SeqUISequencer();

	void ActionDone(const SeqAction action);
	void ActionUndone(const SeqAction action);
	void Draw();

private:
	void DrawSequencerRuler(float height);
	void DrawChannels();
	int TotalChannelHeight();

private:
	const float pixelsPerSecond = 100;
	const int initialChannelHeight = 50;
	SeqProject *project;
	SeqList<int> *channelHeights;
	double position;
	float zoom = 1;
};
