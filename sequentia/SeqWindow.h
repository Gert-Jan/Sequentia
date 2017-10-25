#pragma once

class SeqSerializer;

enum class SeqWindowType
{
	Library,
	Video,
	Sequencer
};

class SeqWindow
{
public:
	virtual SeqWindowType GetWindowType() = 0;
	virtual void Draw() = 0;
	virtual void Serialize(SeqSerializer *serializer) = 0;
};
