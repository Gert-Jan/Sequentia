#pragma once

#include "SeqSerializer.h"
struct SDL_RWops;

class SeqSerializerBin : public SeqSerializer
{
public:
	SeqSerializerBin(SDL_RWops *stream);
	~SeqSerializerBin();

	int GetSerializedVersion();
	void SetSerializedVersion(int version);
	int GetApplicationVersion();
	void SetApplicationVersion(int version);
	
	void Write(const int32_t value);
	int32_t ReadInt();
	void Write(const int64_t value);
	int64_t ReadLong();
	void Write(const float value);
	float ReadFloat();
	void Write(const double value);
	double ReadDouble();
	void Write(const char *string);
	char* ReadString();

private:
	SDL_RWops *stream;

private:
	int serializedVersion;
	int applicationVersion;
};
