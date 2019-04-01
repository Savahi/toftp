#include <windows.h>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "kernel32.lib")
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <map>
#include "ftp.h"
#include "pbar.h"

std::map<int, std::wstring> _errorMessages = {
	{ 0, L"" }, { -1, L"Unknown error"}, 
	{ FTP_ERROR_FAILED_TO_OPEN_INTERNET, L"Failed to open connection. The Internet is unavailable?" }, 
	{ FTP_ERROR_FAILED_TO_CONNECT, L"Failed to connect to server. Please ensure the host address, login, password and port are set correctly." }, 	
	{ FTP_ERROR_FAILED_TO_READ_LOCAL, L"Failed to read local file" },
	{ FTP_ERROR_FAILED_TO_READ_REMOTE, L"Failed to read remote file" }, 
	{ FTP_ERROR_FAILED_TO_WRITE_LOCAL, L"Failed to write local file" }, 
	{ FTP_ERROR_FAILED_TO_WRITE_REMOTE, L"Failed to write remote file" }
};

#define CONNECTION_NAMES_BUFFER 2000
wchar_t _connectionNames[CONNECTION_NAMES_BUFFER + 1];

#define MAX_CONNECTIONS_NUMBER 100
static wchar_t *_connections[MAX_CONNECTIONS_NUMBER];
static int _connectionsNumber = 0;
static int readConnections(wchar_t *fileName);

#define PROFILE_STRING_BUFFER 1000

static wchar_t _server[PROFILE_STRING_BUFFER + 1];
static wchar_t _directory[PROFILE_STRING_BUFFER + 2]; // +2 to append slash if required
static wchar_t _user[PROFILE_STRING_BUFFER + 1];
static wchar_t _password[PROFILE_STRING_BUFFER + 1];
static char _passwordm[PROFILE_STRING_BUFFER + 1];
static wchar_t _mode[PROFILE_STRING_BUFFER + 1];
static int _port = -1;

static int readConnection(wchar_t *fileName, wchar_t *connectionName);

#define MAX_FILES_NUMBER 1000
static wchar_t *_fileNames[MAX_FILES_NUMBER];
static int _filesNumber = 0;
static int readFileNames(wchar_t *fileNamesBuffer);

static void deleteSpacesFromString(wchar_t* str);
static bool isEmptyString(wchar_t* str, bool comma_is_empty_char=false);
static void deleteCharFromString(wchar_t* str, int pos);
static void substituteCharInString(wchar_t*str, wchar_t charToFind, wchar_t charToReplaceWith);
static wchar_t *getPtrToFileName(wchar_t* path);
static void appendDirectoryNameWithEndingSlash(wchar_t *dirName, wchar_t slash);

static void writeErrorIntoIniFile(wchar_t *sectionName, const wchar_t *errorText = nullptr);
static void writeResultIntoIniFile(wchar_t *sectionName, const wchar_t *errors, std::vector<std::wstring>& errorTexts);
static int getTotalNumberOfFilesToTransfer( void );

static int decrypt(char *src, wchar_t *dst);

static wchar_t **_argList = nullptr;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* cmdLine, int nCmdShow)
{
	int exitStatus = -1;
	int status;

	int argCount;
	_argList = CommandLineToArgvW(GetCommandLineW(), &argCount);
	if (_argList == nullptr || argCount < 3) {
		goto lab_exit;
	}

	status = readConnections(_argList[1]);
	if (status <= 0) {
		goto lab_exit;
	}
	
	// The *_connections[] array is initialized now with connection names, the _connectionsNumber variable stores the number of connections read.

	int totalFilesToTransfer = getTotalNumberOfFilesToTransfer();
	if (totalFilesToTransfer == 0) {
		exitStatus = 0;
		goto lab_exit;
	}
	int filesTransferedCounter = 0;

	status = readConnections(_argList[1]);

	HWND progressBarParent=NULL;
	int handle=0;
	if ( argCount >= 4 ) {
		status = swscanf( _argList[3], L"%d", &handle );
		if( status != 1 ) {
			handle = 0;
		}
	} 
	// int handle = GetPrivateProfileIntW(_connections[0], L"Handle", 0, _argList[1]);
	if( handle != 0 ) {
		progressBarParent = (HWND)handle;
	}
	HWND hProgressBar = pbarCreate(hInstance, totalFilesToTransfer+1, progressBarParent);
	pbarStep(hProgressBar);

	for (int iconn = 0; iconn < _connectionsNumber; iconn++) { // Iterating through transfer (connection) sections...
		wchar_t action[PROFILE_STRING_BUFFER + 1];
		status = GetPrivateProfileStringW(_connections[iconn], L"Action", NULL, action, PROFILE_STRING_BUFFER, _argList[1]);
		if (status <= 0 || status >= PROFILE_STRING_BUFFER - 2) {
			writeErrorIntoIniFile(_connections[iconn], L"Unknown action");
			continue;
		}

		if (readConnection(_argList[2], _connections[iconn]) == -1) { // Reading details of the connection
			writeErrorIntoIniFile(_connections[iconn], L"Failed to read connection credentials");
			continue;
		}

		int transferMode;
		if (wcscmp(_mode, L"FTP") == 0) {
			transferMode = 1;
		} else if ((wcscmp(_mode, L"SSH") == 0) || (wcscmp(_mode, L"SFTP") == 0)) {
			writeErrorIntoIniFile(_connections[iconn], L"SSH protocol is not supported");
			continue;
		} else {
			writeErrorIntoIniFile(_connections[iconn], L"This transfer mode is not supported");
			continue;
		}

		wchar_t localDir[PROFILE_STRING_BUFFER + 2]; // A local directory to read file from / write files to
		status = GetPrivateProfileStringW(_connections[iconn], L"LocalDir", NULL, localDir, PROFILE_STRING_BUFFER, _argList[1]);
		if (status <= 0 || status >= PROFILE_STRING_BUFFER - 2) {
			localDir[0] = L'\x0';
			//writeErrorIntoIniFile(_connections[iconn]);
			//continue;
		}
		appendDirectoryNameWithEndingSlash(localDir, L'\\');

		bool remoteDirIsEmpty = false;
		wchar_t remoteDir[PROFILE_STRING_BUFFER + 2]; // A remote directory to read file from / write files to
		status = GetPrivateProfileStringW(_connections[iconn], L"RemoteDir", NULL, remoteDir, PROFILE_STRING_BUFFER, _argList[1]);
		if (status >= PROFILE_STRING_BUFFER - 2) {
			writeErrorIntoIniFile(_connections[iconn], L"Failed to read remote directory");
			continue;
		}
		if (status <= 0 ) {
			remoteDir[0] = L'\x0';
			remoteDirIsEmpty = true;
		}
		else if (isEmptyString(remoteDir)) {
			remoteDir[0] = L'\x0';
			remoteDirIsEmpty = true;
		}
		appendDirectoryNameWithEndingSlash(remoteDir, L'/');

		wchar_t fileNamesBuffer[PROFILE_STRING_BUFFER + 1]; // A buffer to read the list of files into
		status = GetPrivateProfileStringW(_connections[iconn], L"FileNames", NULL, fileNamesBuffer, PROFILE_STRING_BUFFER, _argList[1]);
		if (status <= 0 || status >= PROFILE_STRING_BUFFER - 2) {
			writeErrorIntoIniFile(_connections[iconn], L"Failed to read file names");
			continue;
		}

		int actionCode;
		if (wcscmp(action, L"PUT") == 0) {
			actionCode = 2; // Upload
		} else if (wcscmp(action, L"GET") == 0) {
			actionCode = 1; // Download
		} else if (wcscmp(action, L"DEL") == 0) {
			actionCode = 3; // Delete
		} else {
			continue;
		}

		wchar_t passwordDecrypted[PROFILE_STRING_BUFFER + 1];
		status = decrypt(_passwordm, passwordDecrypted);
		if (status == -1) {
			writeErrorIntoIniFile(_connections[iconn], L"Failed to decrypt password");
			continue;
		}

		wchar_t fullRemoteDir[PROFILE_STRING_BUFFER * 2 + 1];
		wcscpy(fullRemoteDir, _directory);
		appendDirectoryNameWithEndingSlash(fullRemoteDir, L'/');
		if( remoteDir[0] != L'/' ) {
			wcscat(fullRemoteDir, remoteDir);
		} else {
			wcscat(fullRemoteDir, &remoteDir[1]);
		}

		int status = ftpSetCredentials(_server, _user, passwordDecrypted, _port);
		if (status < 0) {
			writeErrorIntoIniFile(_connections[iconn], L"Failed to set credentials");
			continue;
		}
		status = ftpInit();
		if (status < 0) {
			//wchar_t b[1000];
			//wsprintfW( b, L"status=%d", status);
			//MessageBoxW(NULL, b, L"ERROR", MB_OK );
			writeErrorIntoIniFile(_connections[iconn], _errorMessages.find(status)->second.c_str());
			continue;
		}

		if (readFileNames(fileNamesBuffer) <= 0) {
			writeErrorIntoIniFile(_connections[iconn], L"Failed to parse file names to transfer or there are none");
			continue;
		}
			
		wchar_t errors[MAX_FILES_NUMBER + 1];
		for (int ifile = 0; ifile < _filesNumber; ifile++) {
			errors[ifile] = L'-';
		}
		errors[_filesNumber] = L'\x0';
		std::vector<std::wstring> errorTexts;

		for (int ifile = 0; ifile < _filesNumber; ifile++) {
			if (actionCode == 2) { 			// Uploading...
				wchar_t srcPath[PROFILE_STRING_BUFFER * 2 + 1];
				wcscpy(srcPath, localDir);
				wchar_t *fileName = getPtrToFileName(_fileNames[ifile]);
				wcscat(srcPath, fileName);
				substituteCharInString(srcPath, '/', '\\');

				substituteCharInString(_fileNames[ifile], '\\', '/');

				int error;
				status = ftpUpload(srcPath, fileName, fullRemoteDir, !remoteDirIsEmpty);
				ftpGetLastError(&error, NULL, NULL);
				errors[ifile] = (status == 0) ? L'+' : L'-';
				errorTexts.push_back(_errorMessages.find(error)->second);
				
			} else if (actionCode == 1) { 	// Downloading...
				wchar_t destPath[PROFILE_STRING_BUFFER * 2 + 1];
				wcscpy(destPath, localDir);
				wchar_t *fileName = getPtrToFileName(_fileNames[ifile]);
				wcscat(destPath, fileName);
				substituteCharInString(destPath, '/', '\\');

				substituteCharInString(_fileNames[ifile], '\\', '/');

				//MessageBoxW( NULL, destPath, L"E", MB_OK );
				//MessageBoxW( NULL, fileName, L"E", MB_OK );
				//MessageBoxW( NULL, fullRemoteDir, L"E", MB_OK );
				
				int error;
				status = ftpDownload(destPath, fileName, fullRemoteDir);
				ftpGetLastError(&error, NULL, NULL);
				errors[ifile] = (status == 0) ? L'+' : L'-';
				errorTexts.push_back(_errorMessages.find(error)->second);

			} else if( actionCode == 3 ) { 		// Delete
				substituteCharInString(_fileNames[ifile], '\\', '/');

				int error;
				status = ftpDelete(_fileNames[ifile], fullRemoteDir);
				ftpGetLastError(&error, NULL, NULL);
				errors[ifile] = (status == 0) ? L'+' : L'-';
				errorTexts.push_back(_errorMessages.find(error)->second);
			}
			filesTransferedCounter += 1;
			pbarStep(hProgressBar);
		}
		writeResultIntoIniFile(_connections[iconn], errors, errorTexts);

		ftpClose();
	}

	pbarDestroy(hProgressBar);

	exitStatus = 0;

lab_exit:
	if (_argList != nullptr) {
		LocalFree(_argList);
	}

	return exitStatus;
}


int readConnections(wchar_t *fileName)
{
	DWORD charsRead = GetPrivateProfileSectionNamesW(_connectionNames, CONNECTION_NAMES_BUFFER, fileName);
	if (charsRead <= 0 || charsRead == CONNECTION_NAMES_BUFFER - 2) {
		return 0;
	}

	_connections[0] = &_connectionNames[0];
	_connectionsNumber = 1;
	for (unsigned int i = 0; i <= charsRead; i++) {
		if (_connectionNames[i] == L'\x0' && _connectionNames[i + 1] != L'\x0') {
			_connections[_connectionsNumber] = &_connectionNames[i + 1];
			_connectionsNumber++;
			if (_connectionsNumber >= MAX_CONNECTIONS_NUMBER) {
				break;
			}
			i++;
		}
	}
	return _connectionsNumber;
}


int readConnection(wchar_t *fileName, wchar_t *connectionName)
{
	DWORD status;

	status = GetPrivateProfileStringW(connectionName, L"Host", NULL, _server, PROFILE_STRING_BUFFER, fileName);
	if (status <= 0 || status >= PROFILE_STRING_BUFFER - 2) {
		return -1;
	}

	int serverNameLength = wcslen(_server);
	int directoryFoundAt = -1; // A starting directory '/' symbol position - to separate directory from server address
	for (int i = 0; i < serverNameLength; i++) {
		if (_server[i] == L'/') { // A host name contains as well a directory...
			directoryFoundAt = i;
			break;
		}
	}
	if (directoryFoundAt > 0) { // A directory found...
		_server[directoryFoundAt] = L'\x0';
		_directory[0] = L'/';
		int directoryNameLength = 1;
		for (int i = directoryFoundAt + 1; i < serverNameLength; i++) {
			_directory[directoryNameLength] = _server[i];
			directoryNameLength++;
		}
		if (_directory[directoryNameLength - 1] != L'/') {
			_directory[directoryNameLength] = L'/';
			directoryNameLength++;
		}
		_directory[directoryNameLength] = L'\x0';
	}
	else {
		_directory[0] = L'\x0';
	}

	status = GetPrivateProfileStringW(connectionName, L"User", NULL, _user, PROFILE_STRING_BUFFER, fileName);
	if (status <= 0 || status >= PROFILE_STRING_BUFFER - 2) {
		return -1;
	}
	status = GetPrivateProfileStringW(connectionName, L"Password2", NULL, _password, PROFILE_STRING_BUFFER, fileName);
	if (status <= 0 || status >= PROFILE_STRING_BUFFER - 2) {
		return -1;
	}
	char default_char = '?';
	WideCharToMultiByte(CP_ACP, 0, _password, -1, _passwordm, PROFILE_STRING_BUFFER, &default_char, NULL);
	
	status = GetPrivateProfileStringW(connectionName, L"Mode", NULL, _mode, PROFILE_STRING_BUFFER, fileName);
	if (status <= 0 || status >= PROFILE_STRING_BUFFER - 2) {
		return -1;
	}
	_port = GetPrivateProfileIntW(connectionName, L"Port", -1, fileName);
	
	return 0;
}


int readFileNames(wchar_t *fileNamesBuffer) {

	deleteSpacesFromString(fileNamesBuffer);
	if (isEmptyString(fileNamesBuffer, true)) {
		return 0;
	}

	int fileNamesBufferLength = wcslen(fileNamesBuffer);

	_fileNames[0] = fileNamesBuffer;
	_filesNumber = 1;

	for (int ibuff = 0; ibuff < fileNamesBufferLength; ibuff++) {
		if (fileNamesBuffer[ibuff] == L',') { // A separation comma found
			fileNamesBuffer[ibuff] = L'\x0';
			ibuff++;
			_fileNames[_filesNumber] = &fileNamesBuffer[ibuff];
			_filesNumber++;
			if (_filesNumber >= MAX_FILES_NUMBER) {
				break;
			}
		}
	}
	return _filesNumber;
}


static void substituteCharInString(wchar_t*str, wchar_t charToFind, wchar_t charToReplaceWith)
{
	for (unsigned int i = 0; i < wcslen(str); i++) {
		if (str[i] == charToFind) {
			str[i] = charToReplaceWith;
		}
	}
}


static bool isEmptyString(wchar_t* str, bool comma_is_empty_char)
{
	for (unsigned int i = 0; i < wcslen(str); i++) {
		if (str[i] != L' ' && str[i] != L'\r' && str[i] != L'\n' ) {
			return false;
		}
		if( str[i] == L',' && !comma_is_empty_char) {
			return false;
		}
	}
	return true;
}

static void deleteCharFromString(wchar_t* str, int pos)
{
	size_t len = wcslen(str);

	for (unsigned int i = pos + 1; i < len; i++) {
		str[i - 1] = str[i];
	}
	str[len - 1] = L'\x0';
}

static void deleteSpacesFromString(wchar_t* str)
{
	size_t len = wcslen(str);
	for (unsigned int i = 0; i < len; i++) { // Deleting from the beginning
		if (str[i] != L' ') {
			break;
		}
		deleteCharFromString(str, 0);
		len--;
	}

	for (int i = len - 1; i >= 0; i--) { // Deleting from the end
		if (str[i] != L' ') {
			break;
		}
		deleteCharFromString(str, i);
		len--;
	}

	for (unsigned int i = len - 1; i > 0; i--) { // Deleting before ","
		if (str[i - 1] == L' ' && str[i] == L',') {
			deleteCharFromString(str, i - 1);
			len--;
		}
	}

	for (unsigned int i = 1; i < len; ) { // Deleting after ","
		if (str[i - 1] == L',' && str[i] == L' ') {
			deleteCharFromString(str, i);
			len--;
		}
		else {
			i++;
		}
	}
}


static wchar_t *getPtrToFileName(wchar_t* path)
{
	wchar_t *ptr = &path[0];

	size_t len = wcslen(path);
	for (int i = len - 2; i >= 0; i--) { // Starting from the end...
		if ( (path[i] == L'\\' || path[i] == L'/') && (path[i+1] != L'\\' && path[i+1] != L'/') ) {
			ptr = &path[i+1];
			break;
		}
	}
	return ptr;
}

static void writeErrorIntoIniFile(wchar_t *sectionName, const wchar_t *errorText)
{
	wchar_t *defaultErrorText = L"Error";
	if (errorText == nullptr) {
		errorText = defaultErrorText;
	}
	WritePrivateProfileStringW(sectionName, L"Result", L"-", _argList[1]);
	WritePrivateProfileStringW(sectionName, L"Reason", errorText, _argList[1]);
}


static void writeResultIntoIniFile(wchar_t *sectionName, const wchar_t *errors, std::vector<std::wstring>& errorTexts)
{
	WritePrivateProfileStringW(sectionName, L"Result", errors, _argList[1]);

	std::wstring errorTextsCombined;
	for (int i = 0; i < errorTexts.size(); i++) {
		if (i > 0) {
			errorTextsCombined.append(L";");
		}
		errorTextsCombined.append(errorTexts[i]);
	}
	WritePrivateProfileStringW(sectionName, L"Reason", errorTextsCombined.c_str(), _argList[1]);
}


static int decrypt(char *src, wchar_t *dst) {
    const char *xorkey1b= "_23ken08SPIDER1970&%_23ken08SPIDER1970&%\0";
    int xorkey1bLen = strlen(xorkey1b);
    wchar_t *xorkey = (wchar_t *)xorkey1b;
    int xorkeyLen = (xorkey1bLen-1)/2;

	int passwordLength = strlen(src);
	if (passwordLength % 4) {
		return -1;
	}

	char symbolBuffer[5];
	symbolBuffer[4] = '\x0';
	
	for (int iSrc = 0, iDst = 0; iSrc < passwordLength ; iSrc += 4, iDst++) {
		symbolBuffer[0] = src[iSrc + 0];
		symbolBuffer[1] = src[iSrc + 1];
		symbolBuffer[2] = src[iSrc + 2];
		symbolBuffer[3] = src[iSrc + 3];
		unsigned short dec;
		int status = sscanf(symbolBuffer, "%hx", &dec);
		//cout << symbolBuffer << ", dec=" << dec << endl;
		if (status != 1) {
			return -1;
		}
		if( iDst < xorkeyLen ) {
			dst[iDst] = (wchar_t)(dec ^ (unsigned short)xorkey[iDst]);			
		} else {
			dst[iDst] = (wchar_t)(dec);						
		}
	}
	dst[passwordLength/4] = L'\x0';
	return 0;
}


static int getTotalNumberOfFilesToTransfer(void)
{
	int filesTotal = 0;

	for (int iconn = 0; iconn < _connectionsNumber; iconn++) { // Iterating through transfer (connection) sections...
		wchar_t fileNamesBuffer[PROFILE_STRING_BUFFER + 1];
		int status = GetPrivateProfileStringW(_connections[iconn], L"FileNames", NULL, fileNamesBuffer, PROFILE_STRING_BUFFER, _argList[1]);
		if (status <= 0 || status >= PROFILE_STRING_BUFFER - 2) {
			continue;
		}
		int filesNumber = readFileNames(fileNamesBuffer);
		if (filesNumber <= 0) {
			continue;
		}
		filesTotal += filesNumber;
	}
	return filesTotal;
}

static void appendDirectoryNameWithEndingSlash(wchar_t *dirName, wchar_t slash)
{
	int dirNameLen = wcslen(dirName);
	if (dirNameLen > 0) {
		if (dirName[dirNameLen - 1] != slash) {
			dirName[dirNameLen] = slash;
			dirName[dirNameLen + 1] = L'\x0';
		}
	}
}
