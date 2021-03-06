#include <SDL.h>
#include <string>
#include "SeqSerializerBin.h"

SeqSerializerBin::SeqSerializerBin(SDL_RWops *stream):
	stream(stream)
{
}

SeqSerializerBin::~SeqSerializerBin()
{
}

int SeqSerializerBin::GetSerializedVersion()
{
	return serializedVersion;
}

void SeqSerializerBin::SetSerializedVersion(int version)
{
	serializedVersion = version;
}

int SeqSerializerBin::GetApplicationVersion()
{
	return applicationVersion;
}

void SeqSerializerBin::SetApplicationVersion(int version)
{
	applicationVersion = version;
}

void SeqSerializerBin::Write(const int32_t value)
{
	stream->write(stream, &value, 4, 1);
}

int32_t SeqSerializerBin::ReadInt()
{
	int value = 0;
	stream->read(stream, &value, 4, 1);
	return value;
}

void SeqSerializerBin::Write(const int64_t value)
{
	stream->write(stream, &value, 8, 1);
}

int64_t SeqSerializerBin::ReadLong()
{
	int64_t value = 0;
	stream->read(stream, &value, 8, 1);
	return value;
}

void SeqSerializerBin::Write(const float value)
{
	// TODO: Make sure to have a standard binary representation for floats. 
	stream->write(stream, &value, 4, 1);
}

float SeqSerializerBin::ReadFloat()
{
	// TODO: Make sure to have a standard binary representation for floats.
	float value = 0;
	stream->read(stream, &value, 4, 1);
	return value;
}

void SeqSerializerBin::Write(const double value)
{
	// TODO: Make sure to have a standard binary representation for doubles.
	stream->write(stream, &value, 8, 1);
}

double SeqSerializerBin::ReadDouble()
{
	// TODO: Make sure to have a standard binary representation for doubles.
	double value = 0;
	stream->read(stream, &value, 8, 1);
	return value;
}

void SeqSerializerBin::Write(const char *string)
{
	// TODO: Research if everything will work well here in case UTF-8 encoding is used.
	int length = strlen(string);
	Write(length);
	stream->write(stream, string, 1, length);
}

char* SeqSerializerBin::ReadString()
{
	// TODO: Research if everything will work well here in case UTF-8 encoding is used.
	int length = ReadInt();
	char *string = new char[length + 1];
	stream->read(stream, string, 1, length);
	string[length] = 0;
	return string;
}