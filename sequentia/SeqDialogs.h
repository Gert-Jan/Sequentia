#pragma once

class SeqProject;

enum class RequestPathAction
{
	Save,
	Open,
	AddToLibrary
};

enum SeqDialogOption
{
	OK		= (1u << 0),
	Cancel	= (1u << 1),
	Yes		= (1u << 2),
	No		= (1u << 3)
};

class SeqDialogs
{
public:
	static void ShowError(char *message, ...);
	static void ShowRequestProjectPath(char *currentPath, RequestPathAction action);
	static void Draw(SeqProject *project);

private:
	static bool ShowFileBrowseDialog(const char *title);
	static bool ShowMessage(const char *title, int options);
	static bool ShowMessageButton(const char *label, int options, SeqDialogOption option);

private:
	static bool showRequestProjectPath;
	static bool showWarningOverwrite;
	static bool showError;

	static SeqDialogOption result;
	static RequestPathAction requestPathAction;
	static char *path;
	static char *message;
};