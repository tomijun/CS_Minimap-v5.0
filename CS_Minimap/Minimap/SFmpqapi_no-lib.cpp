#include "SFmpqapi_no-lib.h"
#include <iostream>
std::wstring s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}

struct SFMPQAPIMODULE {
	SFMPQAPIMODULE();
	~SFMPQAPIMODULE();
} SFMpqApi;

HINSTANCE hSFMpq = 0;

funcMpqInitialize MpqInitialize = 0;

funcMpqGetVersionString MpqGetVersionString = 0;
funcMpqGetVersion MpqGetVersion = 0;

funcSFMpqGetVersionString SFMpqGetVersionString = 0;
funcSFMpqGetVersionString2 SFMpqGetVersionString2 = 0;
funcSFMpqGetVersion SFMpqGetVersion = 0;

funcSFileOpenArchive SFileOpenArchive = 0;
funcSFileCloseArchive SFileCloseArchive = 0;
funcSFileGetArchiveName SFileGetArchiveName = 0;
funcSFileOpenFile SFileOpenFile = 0;
funcSFileOpenFileEx SFileOpenFileEx = 0;
funcSFileCloseFile SFileCloseFile = 0;
funcSFileGetFileSize SFileGetFileSize = 0;
funcSFileGetFileArchive SFileGetFileArchive = 0;
funcSFileGetFileName SFileGetFileName = 0;
funcSFileSetFilePointer SFileSetFilePointer = 0;
funcSFileReadFile SFileReadFile = 0;
funcSFileSetLocale SFileSetLocale = 0;
funcSFileGetBasePath SFileGetBasePath = 0;
funcSFileSetBasePath SFileSetBasePath = 0;

funcSFileGetFileInfo SFileGetFileInfo = 0;
funcSFileSetArchivePriority SFileSetArchivePriority = 0;
funcSFileFindMpqHeader SFileFindMpqHeader = 0;
funcSFileListFiles SFileListFiles = 0;

funcMpqOpenArchiveForUpdate MpqOpenArchiveForUpdate = 0;
funcMpqCloseUpdatedArchive MpqCloseUpdatedArchive = 0;
funcMpqAddFileToArchive MpqAddFileToArchive = 0;
funcMpqAddWaveToArchive MpqAddWaveToArchive = 0;
funcMpqRenameFile MpqRenameFile = 0;
funcMpqDeleteFile MpqDeleteFile = 0;
funcMpqCompactArchive MpqCompactArchive = 0;

funcMpqAddFileToArchiveEx MpqAddFileToArchiveEx = 0;
funcMpqAddFileFromBufferEx MpqAddFileFromBufferEx = 0;
funcMpqAddFileFromBuffer MpqAddFileFromBuffer = 0;
funcMpqAddWaveFromBuffer MpqAddWaveFromBuffer = 0;
funcMpqSetFileLocale MpqSetFileLocale = 0;

funcSFileDestroy SFileDestroy = 0;
funcStormDestroy StormDestroy = 0;

typedef void (WINAPI* funcSFMpqDestroy)();
funcSFMpqDestroy SFMpqDestroy = 0;

SFMPQAPIMODULE::SFMPQAPIMODULE()
{
	if (hSFMpq!=0) return;
	std::wstring stemp = s2ws("SFMpq.dll");
	LPCWSTR SFMPQName = stemp.c_str();
	hSFMpq = LoadLibrary(SFMPQName);

	if (hSFMpq!=0) {
		MpqInitialize = (funcMpqInitialize)GetProcAddress(hSFMpq,"MpqInitialize");

		MpqGetVersionString = (funcMpqGetVersionString)GetProcAddress(hSFMpq,"MpqGetVersionString");
		MpqGetVersion = (funcMpqGetVersion)GetProcAddress(hSFMpq,"MpqGetVersion");

		SFMpqGetVersionString = (funcSFMpqGetVersionString)GetProcAddress(hSFMpq,"SFMpqGetVersionString");
		SFMpqGetVersionString2 = (funcSFMpqGetVersionString2)GetProcAddress(hSFMpq,"SFMpqGetVersionString2");
		SFMpqGetVersion = (funcSFMpqGetVersion)GetProcAddress(hSFMpq,"SFMpqGetVersion");

		SFileOpenArchive = (funcSFileOpenArchive)GetProcAddress(hSFMpq,"SFileOpenArchive");
		SFileCloseArchive = (funcSFileCloseArchive)GetProcAddress(hSFMpq,"SFileCloseArchive");
		SFileGetArchiveName = (funcSFileGetArchiveName)GetProcAddress(hSFMpq,"SFileGetArchiveName");
		SFileOpenFile = (funcSFileOpenFile)GetProcAddress(hSFMpq,"SFileOpenFile");
		SFileOpenFileEx = (funcSFileOpenFileEx)GetProcAddress(hSFMpq,"SFileOpenFileEx");
		SFileCloseFile = (funcSFileCloseFile)GetProcAddress(hSFMpq,"SFileCloseFile");
		SFileGetFileSize = (funcSFileGetFileSize)GetProcAddress(hSFMpq,"SFileGetFileSize");
		SFileGetFileArchive = (funcSFileGetFileArchive)GetProcAddress(hSFMpq,"SFileGetFileArchive");
		SFileGetFileName = (funcSFileGetFileName)GetProcAddress(hSFMpq,"SFileGetFileName");
		SFileSetFilePointer = (funcSFileSetFilePointer)GetProcAddress(hSFMpq,"SFileSetFilePointer");
		SFileReadFile = (funcSFileReadFile)GetProcAddress(hSFMpq,"SFileReadFile");
		SFileSetLocale = (funcSFileSetLocale)GetProcAddress(hSFMpq,"SFileSetLocale");
		SFileGetBasePath = (funcSFileGetBasePath)GetProcAddress(hSFMpq,"SFileGetBasePath");
		SFileSetBasePath = (funcSFileSetBasePath)GetProcAddress(hSFMpq,"SFileSetBasePath");

		SFileGetFileInfo = (funcSFileGetFileInfo)GetProcAddress(hSFMpq,"SFileGetFileInfo");
		SFileSetArchivePriority = (funcSFileSetArchivePriority)GetProcAddress(hSFMpq,"SFileSetArchivePriority");
		SFileFindMpqHeader = (funcSFileFindMpqHeader)GetProcAddress(hSFMpq,"SFileFindMpqHeader");
		SFileListFiles = (funcSFileListFiles)GetProcAddress(hSFMpq,"SFileListFiles");

		MpqOpenArchiveForUpdate = (funcMpqOpenArchiveForUpdate)GetProcAddress(hSFMpq,"MpqOpenArchiveForUpdate");
		MpqCloseUpdatedArchive = (funcMpqCloseUpdatedArchive)GetProcAddress(hSFMpq,"MpqCloseUpdatedArchive");
		MpqAddFileToArchive = (funcMpqAddFileToArchive)GetProcAddress(hSFMpq,"MpqAddFileToArchive");
		MpqAddWaveToArchive = (funcMpqAddWaveToArchive)GetProcAddress(hSFMpq,"MpqAddWaveToArchive");
		MpqRenameFile = (funcMpqRenameFile)GetProcAddress(hSFMpq,"MpqRenameFile");
		MpqDeleteFile = (funcMpqDeleteFile)GetProcAddress(hSFMpq,"MpqDeleteFile");
		MpqCompactArchive = (funcMpqCompactArchive)GetProcAddress(hSFMpq,"MpqCompactArchive");

		MpqAddFileToArchiveEx = (funcMpqAddFileToArchiveEx)GetProcAddress(hSFMpq,"MpqAddFileToArchiveEx");
		MpqAddFileFromBufferEx = (funcMpqAddFileFromBufferEx)GetProcAddress(hSFMpq,"MpqAddFileFromBufferEx");
		MpqAddFileFromBuffer = (funcMpqAddFileFromBuffer)GetProcAddress(hSFMpq,"MpqAddFileFromBuffer");
		MpqAddWaveFromBuffer = (funcMpqAddWaveFromBuffer)GetProcAddress(hSFMpq,"MpqAddWaveFromBuffer");
		MpqSetFileLocale = (funcMpqSetFileLocale)GetProcAddress(hSFMpq,"MpqSetFileLocale");

		SFileDestroy = (funcSFileDestroy)GetProcAddress(hSFMpq,"SFileDestroy");
		StormDestroy = (funcStormDestroy)GetProcAddress(hSFMpq,"StormDestroy");

		SFMpqDestroy = (funcSFMpqDestroy)GetProcAddress(hSFMpq,"SFMpqDestroy");
	}
}

SFMPQAPIMODULE::~SFMPQAPIMODULE()
{
	MpqInitialize = 0;

	MpqGetVersionString = 0;
	MpqGetVersion = 0;

	SFMpqGetVersionString = 0;
	SFMpqGetVersionString2 = 0;
	SFMpqGetVersion = 0;

	SFileOpenArchive = 0;
	SFileCloseArchive = 0;
	SFileGetArchiveName = 0;
	SFileOpenFile = 0;
	SFileOpenFileEx = 0;
	SFileCloseFile = 0;
	SFileGetFileSize = 0;
	SFileGetFileArchive = 0;
	SFileGetFileName = 0;
	SFileSetFilePointer = 0;
	SFileReadFile = 0;
	SFileSetLocale = 0;
	SFileGetBasePath = 0;
	SFileSetBasePath = 0;

	SFileGetFileInfo = 0;
	SFileSetArchivePriority = 0;
	SFileFindMpqHeader = 0;
	SFileListFiles = 0;

	MpqOpenArchiveForUpdate = 0;
	MpqCloseUpdatedArchive = 0;
	MpqAddFileToArchive = 0;
	MpqAddWaveToArchive = 0;
	MpqRenameFile = 0;
	MpqDeleteFile = 0;
	MpqCompactArchive = 0;

	MpqAddFileToArchiveEx = 0;
	MpqAddFileFromBufferEx = 0;
	MpqAddFileFromBuffer = 0;
	MpqAddWaveFromBuffer = 0;
	MpqSetFileLocale = 0;

	SFileDestroy = 0;
	StormDestroy = 0;

	if (hSFMpq==0) return;
	if (SFMpqDestroy!=0) SFMpqDestroy();

	SFMpqDestroy = 0;

	FreeLibrary(hSFMpq);
	hSFMpq = 0;
}

void LoadSFMpq()
{
}

void FreeSFMpq()
{
}

long SFMpqCompareVersion()
{
	SFMPQVERSION ExeVersion = {1,0,7,4};
	SFMPQVERSION DllVersion = SFMpqGetVersion();
	if (DllVersion.Major>ExeVersion.Major) return 1;
	else if (DllVersion.Major<ExeVersion.Major) return -1;
	if (DllVersion.Minor>ExeVersion.Minor) return 1;
	else if (DllVersion.Minor<ExeVersion.Minor) return -1;
	if (DllVersion.Revision>ExeVersion.Revision) return 1;
	else if (DllVersion.Revision<ExeVersion.Revision) return -1;
	if (DllVersion.Subrevision>ExeVersion.Subrevision) return 1;
	else if (DllVersion.Subrevision<ExeVersion.Subrevision) return -1;
	return 0;
}

