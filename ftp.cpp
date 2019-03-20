#include <windows.h>
#include <wininet.h>
#include <string.h>
#pragma comment(lib, "Wininet")
#include "ftp.h"

#define FTP_MAX_SERVER 100
static wchar_t _server[FTP_MAX_SERVER + 1];

#define FTP_MAX_USER 100
static wchar_t _user[FTP_MAX_USER + 1];

#define FTP_MAX_PASSWORD 100
static wchar_t _password[FTP_MAX_PASSWORD + 1];

#define FTP_MAX_REMOTE_ADDR 500
static wchar_t _remoteAddr[FTP_MAX_REMOTE_ADDR + 1];

static unsigned long _port;

static long int _ftpErrorCode = 0;
static DWORD _winInetErrorCode = 0;
static wchar_t *_winInetErrorText = NULL;

static long int _timeOut = -1L;

static HINTERNET _hInternet = NULL;
static HINTERNET _hFtpSession = NULL;

static int validateDirectories(wchar_t *);

static int createRemoteAddr(wchar_t *fileName, wchar_t *directory, wchar_t *server, wchar_t *user, wchar_t *password)
{
	if( server != NULL && user != NULL && password != NULL ) {	
		if (wcslen(server) + wcslen(user) + wcslen(password) + wcslen(directory) + wcslen(fileName) + 7 >= FTP_MAX_REMOTE_ADDR) {
			return -1;
		}
		wcscpy(_remoteAddr, L"ftp://");
		wcscpy(_remoteAddr, user);
		wcscpy(_remoteAddr, L":");
		wcscpy(_remoteAddr, password);
		wcscpy(_remoteAddr, L"@");
		wcscpy(_remoteAddr, server);
		wcscpy(_remoteAddr, directory);
		wcscpy(_remoteAddr, L"/");
		wcscpy(_remoteAddr, fileName);
	} else {
		int directoryLength = wcslen(directory);
		int fileNameLength = wcslen(fileName);
		if (directoryLength + fileNameLength + 1 >= FTP_MAX_REMOTE_ADDR) {
			return -1;
		}
		wcscpy(_remoteAddr, directory);
		if (directoryLength > 0) {
			if (_remoteAddr[directoryLength - 1] != '/') {
				wcscat(_remoteAddr, L"/");
			}
		}
		wcscat(_remoteAddr, fileName);					
	}
	return 0;
}



int ftpTest(wchar_t *fileName, wchar_t *directory, unsigned long int *size)
{
	_ftpErrorCode = 0;
	_winInetErrorCode = 0;

	return 1;
}


int ftpDelete(wchar_t *dstFileName, wchar_t *dstDirectory ) 
{
	_ftpErrorCode = 0;
	_winInetErrorCode = 0;

	if (createRemoteAddr(dstFileName, dstDirectory, NULL, NULL, NULL) == -1) {
		_ftpErrorCode = -1;
	} else {
		int lastCharIndex = wcslen(_remoteAddr) - 1;
		if( _remoteAddr[lastCharIndex] != L'*' ) {
			//MessageBoxW( NULL, _remoteAddr, L"FILE TO DELETE", MB_OK );
			//if( !FtpDeleteFileW(_hFtpSession, _remoteAddr) ) {
			//	_ftpErrorCode = -1;				
			//}
		} else {
			WIN32_FIND_DATAW fd;
			HINTERNET hFtpSession = InternetConnectW(_hInternet, _server, 
				INTERNET_DEFAULT_FTP_PORT, _user, _password, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
			if (hFtpSession) {
				HINTERNET hFind = FtpFindFirstFileW(_hFtpSession,_remoteAddr,&fd,0,0);
				if( hFind ) {
					bool findNext;
					do {
						if( fd.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY ) {
							wchar_t fileToDelete[FTP_MAX_REMOTE_ADDR + 1 + MAX_PATH];
							wcscpy(fileToDelete, _remoteAddr);
							fileToDelete[lastCharIndex] = '\x0';
							wcscat( fileToDelete, fd.cFileName );
							//MessageBoxW( NULL, fileToDelete, L"FILE TO DELETE", MB_OK );
							//if( !FtpDeleteFileW(_hFtpSession, fileToDelete) ) {
							//	_ftpErrorCode = -1;
							//}
						}
						findNext = InternetFindNextFileW(hFind, &fd);
					} while( findNext );
					InternetCloseHandle(hFind);
				}
				InternetCloseHandle(hFtpSession);
			} else {
				_ftpErrorCode = -1;
			}
		}
	}
	return _ftpErrorCode;
}


int ftpUpload(wchar_t *srcFileName, wchar_t *dstFileName, wchar_t *dstDirectory ) 
{
	_ftpErrorCode = 0;
	_winInetErrorCode = 0;

	if (createRemoteAddr(dstFileName, dstDirectory, NULL, NULL, NULL) == -1) {
		_ftpErrorCode = -1;
	} else {
		if (validateDirectories(_remoteAddr) >= 0) {
			DWORD status = FtpPutFileW(_hFtpSession, srcFileName, _remoteAddr, FTP_TRANSFER_TYPE_BINARY, 0);
			if (!status) {
				_ftpErrorCode = -1;
			}
		}
	}
	return _ftpErrorCode;
}


int ftpDownload(wchar_t *dstFileName, wchar_t *srcFileName, wchar_t *srcDirectory ) 
{
	_ftpErrorCode = 0;
	_winInetErrorCode = 0;

	if (createRemoteAddr(srcFileName, srcDirectory, NULL, NULL, NULL) == -1) {
		_ftpErrorCode = -1;
	} else {
		//MessageBoxW(NULL, _remoteAddr, L"R", MB_OK );
		int status = FtpGetFileW(_hFtpSession, _remoteAddr, dstFileName, false, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_BINARY, 0); 
		if( !status ) {
			_ftpErrorCode = -1;
		}
	}
	return _ftpErrorCode;
}


void ftpSetTimeOut(unsigned long int timeOut) {
	_ftpErrorCode = 0;
	_winInetErrorCode = 0;

	_timeOut = timeOut;
}


int ftpSetCredentials(wchar_t *server, wchar_t *user, wchar_t *password, int port) {
	_ftpErrorCode = 0;
	_winInetErrorCode = 0;

	if( wcslen(server) > FTP_MAX_SERVER || wcslen(user) > FTP_MAX_USER || wcslen(password) > FTP_MAX_PASSWORD ) {
		_ftpErrorCode = -1;
	} else {
		wcscpy( _server, server );
		wcscpy( _user, user );
		wcscpy( _password, password );
		_port = port;		
		if( _port < 0 ) {
			_port = INTERNET_DEFAULT_FTP_PORT;
		}
	}
	return _ftpErrorCode;
}


int ftpInit(void) {
	_ftpErrorCode = 0;
	_winInetErrorCode = 0;

	_hInternet = InternetOpenW(NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!_hInternet) {
		_ftpErrorCode = -1;
	} else {
		_hFtpSession = InternetConnectW(_hInternet, _server, INTERNET_DEFAULT_FTP_PORT, _user, _password, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
		if (!_hFtpSession) {
			_ftpErrorCode = -1;
		} 
	}

	if( _ftpErrorCode != 0 ) {
		ftpClose();
	}

	return _ftpErrorCode;
}


void ftpClose(void) {
	_ftpErrorCode = 0;
	_winInetErrorCode = 0;

	if( _hFtpSession != NULL ) {
	    InternetCloseHandle(_hFtpSession);
	}
	_hFtpSession = NULL;
	if( _hInternet != NULL ) {
	    InternetCloseHandle(_hInternet);
	}
	_hInternet = NULL;
}


int ftpGetLastError(int *ftpErrorCode, DWORD *winInetErrorCode, wchar_t *winInetErrorText) {
	if (ftpErrorCode != NULL) {
		*ftpErrorCode = _ftpErrorCode;
	}
	if (winInetErrorCode != NULL) {
		*winInetErrorCode = GetLastError();
	}
	if (winInetErrorText != NULL) {
		winInetErrorText = NULL;
	}
	return 0;
}


static int validateDirectories( wchar_t *remoteAddr ) {
	int returnValue = 0;
	wchar_t remoteDir[FTP_MAX_REMOTE_ADDR + 1];
	WIN32_FIND_DATAW findFileData;

	int remoteAddrLen = wcslen(remoteAddr);

	for (int i = 1; i < remoteAddrLen; i++) {
		if ( (remoteAddr[i] == '\\' || remoteAddr[i] == '/') && (remoteAddr[i-1] != '\\' && remoteAddr[i-1] != '/') ) {
			wcsncpy(remoteDir, remoteAddr, i);
			remoteDir[i] = '\x0';

			HINTERNET hFtpSession = InternetConnectW(_hInternet, _server, 
				INTERNET_DEFAULT_FTP_PORT, _user, _password, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
			if (hFtpSession) {
				bool status;
				if (FtpFindFirstFileW(hFtpSession, remoteDir, &findFileData, 0, NULL) == NULL) { // The directory wasn't found...
					status = FtpCreateDirectoryW(hFtpSession, remoteDir);
				}
				InternetCloseHandle(hFtpSession);
				if (!status) {
					returnValue = -1;
					break;
				}
			}
			else {
				returnValue = -1;
				break;
			}
		}
	}
	return returnValue;
}
