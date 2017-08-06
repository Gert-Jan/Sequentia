#pragma once

#include "SeqList.h";
#include "SeqChannel.h";
#include "SeqAction.h";
#include "SeqUISequencer.h";

class SeqProject
{
public:
	SeqProject();
	~SeqProject();

	void AddAction(SeqAction action);
	void AddActionHandler(SeqActionHandler *handler);
	int GetActionCount();
	int GetActionCursor();
	SeqAction GetAction(const int index);
	void DoAction(SeqAction action);
	void UndoAction(SeqAction action);

	int GetChannelCount();
	SeqChannel GetChannel(const int index);

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
	SeqUISequencer *uiSequencer;
};
