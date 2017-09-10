#include "SeqChannel.h";
#include "SeqSerializer.h";

SeqChannel::SeqChannel()
{
}

SeqChannel::SeqChannel(char *name, SeqChannelType type):
	type(type),
	name(name)
{
}

SeqChannel::SeqChannel(SeqSerializer *serializer)
{
	Deserialize(serializer);
}

SeqChannel::~SeqChannel()
{
}

void SeqChannel::Serialize(SeqSerializer *serializer)
{
	serializer->Write((int)type);
	serializer->Write(name);
}

void SeqChannel::Deserialize(SeqSerializer *serializer)
{
	type = (SeqChannelType)serializer->ReadInt();
	name = serializer->ReadString();
}
