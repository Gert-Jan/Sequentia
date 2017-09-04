#pragma once

class SeqProject;

class SeqDialogs
{
public:
	static void Draw(SeqProject *project);
	static void ShowRequestProjectPath(char *currentPath);
	static void ShowError(char *message, int error);

private:
	static bool showRequestProjectPath;
	static bool showWarningOverwrite;
	static bool showError;

	static char *projectPath;
	static char *errorMessage;
	static int errorNumber;
};