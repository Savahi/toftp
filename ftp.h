// File transfer via SSH
#ifndef __FTP_H
#define __FTP_H

#define FTP_ERROR_Ok 0
#define FTP_ERROR -1
#define FTP_ERROR_FAILED_TO_READ_LOCAL -2
#define FTP_ERROR_FAILED_TO_READ_REMOTE -3
#define FTP_ERROR_FAILED_TO_WRITE_LOCAL -4
#define FTP_ERROR_FAILED_TO_WRITE_REMOTE -5

// Uploads a file to a server. Returns negative value if failed, 0 if ok.
// The connection credentials must be set earlier...
int ftpUpload( wchar_t *srcFileName,					// A file to transfer to a server
	wchar_t *dstFileName, 								// A name for the file when it is stored at the server
	wchar_t *dstDirectory); 							// A directory to transfer the file into. For Linux servers starts with '/'

// Downloads a file from a server. Returns negative value if failed, 0 if ok.
// The connection credentials must be set earlier...
int ftpDownload( wchar_t *dstFileName, 				// A file name to save the downloaded file under 
	wchar_t *srcFileName, 							// A file to download
	wchar_t *srcDirectory); 						// The directory to find the file at the server

int ftpDelete(wchar_t *dstFileName, 
	wchar_t *dstDirectory );
	
	// Test is a file exists at a server. "1" - yes, "0" - no, "-1" - error.
// The connection credentials must be set earlier...
int ftpTest( wchar_t *fileName, 					// A file name to test
	wchar_t *directory, 							// A server directory to test in
  	unsigned long int *size );						// If not NULL receives the size of file in bytes

// 
int ftpSetCredentials(wchar_t *server, wchar_t *user, wchar_t *password, int);

int ftpInit( void ); // Must be called before doing anything else...

void ftpClose( void ); // Must be called when all transfers are finished...

int ftpGetLastError( int *ftpErrorCode, 		// 
	DWORD *winInetErrorCode, 						//
	wchar_t *winInetErrorText );						//

#endif
