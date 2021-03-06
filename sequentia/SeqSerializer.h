#pragma once

#include "SDL_config.h"

class SeqSerializer
{
public:
	virtual int GetSerializedVersion() = 0;
	virtual void SetSerializedVersion(int serializedVersion) = 0;
	virtual int GetApplicationVersion() = 0;
	virtual void SetApplicationVersion(int applicationVersion) = 0;

	virtual void Write(const int32_t value) = 0;
	virtual int32_t ReadInt() = 0;
	virtual void Write(const int64_t value) = 0;
	virtual int64_t ReadLong() = 0;
	virtual void Write(const float value) = 0;
	virtual float ReadFloat() = 0;
	virtual void Write(const double value) = 0;
	virtual double ReadDouble() = 0;
	virtual void Write(const char *string) = 0;
	virtual char* ReadString() = 0;
};