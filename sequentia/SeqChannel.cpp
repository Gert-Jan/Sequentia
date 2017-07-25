#include "SeqChannel.h";

SeqChannel::SeqChannel()
{
}

SeqChannel::SeqChannel(char *name, SeqChannelType type):
	type(type),
	name(name)
{
}

SeqChannel::~SeqChannel()
{
}
