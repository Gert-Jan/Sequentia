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
	
	void Write(int32_t value);
	int32_t ReadInt();
	void Write(int64_t value);
	int64_t ReadLong();
	void Write(float value);
	float ReadFloat();
	void Write(double value);
	double ReadDouble();
	void Write(char *string);
	char* ReadString();

private:
	SDL_RWops *stream;

private:
	int serializedVersion;
	int applicationVersion;
};
