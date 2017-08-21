#pragma once

#include "SeqList.h";
#include "SeqChannel.h";
#include "SeqAction.h";
#include "SeqUISequencer.h";
#include "SeqUILibrary.h";

class SeqProject
{
public:
	SeqProject();
	~SeqProject();

	void AddAction(const SeqAction action);
	void AddActionHandler(SeqActionHandler *handler);
	int GetActionCount();
	int GetActionCursor();
	SeqAction GetAction(const int index);
	void DoAction(const SeqAction action);
	void UndoAction(const SeqAction action);

	int GetChannelCount();
	SeqChannel GetChannel(const int index);

	void AddSequencer();
	void RemoveSequencer(SeqUISequencer *sequencer);
	void AddLibrary();
	void RemoveLibrary(SeqUILibrary *library);

	double GetLength();

	void Draw();

private:
	void AddChannel(SeqChannelType type, char *name);
	void RemoveChannel(const int index);

private:
	SeqList<SeqActionHandler*> *actionHandlers;
	SeqList<SeqAction> *actions;
	int actionCursor = 0;
	SeqList<SeqChannel> *channels;
	SeqList<SeqUISequencer*> *uiSequencers;
	SeqList<SeqUILibrary*> *uiLibraries;
};
