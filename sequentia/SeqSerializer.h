#pragma once

class SeqSerializer
{
public:
	virtual int GetSerializedVersion() = 0;
	virtual void SetSerializedVersion(int serializedVersion) = 0;
	virtual int GetApplicationVersion() = 0;
	virtual void SetApplicationVersion(int applicationVersion) = 0;

	virtual void Write(int value) = 0;
	virtual int ReadInt() = 0;
	virtual void Write(float value) = 0;
	virtual float ReadFloat() = 0;
	virtual void Write(double value) = 0;
	virtual double ReadDouble() = 0;
	virtual void Write(char *string) = 0;
	virtual char* ReadString() = 0;
};