#pragma once

class SeqSerializer;

enum class SeqChannelType
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
	SeqChannel(SeqSerializer *serializer);
	~SeqChannel();

	void Serialize(SeqSerializer *serializer);

private:
	void Deserialize(SeqSerializer *serializer);

public:
	SeqChannelType type;
	char *name;
};
