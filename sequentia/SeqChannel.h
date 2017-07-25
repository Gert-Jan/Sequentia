#pragma once

enum SeqChannelType
{
	None,
	Video,
	Audio
};

class SeqChannel
{
public:
	SeqChannel();
	SeqChannel(char *name, SeqChannelType type);
	~SeqChannel();

public:
	SeqChannelType type;
	char *name;
};
