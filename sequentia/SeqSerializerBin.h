#pragma once

#include "SeqSerializer.h";
struct SDL_RWops;

class SeqSerializerBin : public SeqSerializer
{
public:
	SeqSerializerBin(SDL_RWops *stream);
	~SeqSerializerBin();

	int GetSerializedVersion();
	void SetSerializedVersion(int serializedVersion);
	int GetApplicationVersion();
	void SetApplicationVersion(int applicationVersion);
	
	void Write(int value);
	int ReadInt();
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
