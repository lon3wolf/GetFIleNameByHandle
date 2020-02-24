#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <string.h>
#include <psapi.h>
#include <strsafe.h>

#define BUFSIZE 512

// https://docs.microsoft.com/en-us/windows/win32/memory/obtaining-a-file-name-from-a-file-handle

TCHAR* GetFileNameFromHandle(HANDLE hFile, size_t* d, TCHAR* fileName)
{
  BOOL bSuccess = FALSE;
  TCHAR pszFilename[MAX_PATH+1];
  HANDLE hFileMap;

  // Get the file size.
  DWORD dwFileSizeHi = 0;
  DWORD dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi); 

  if( dwFileSizeLo == 0 && dwFileSizeHi == 0 )
  {
     _tprintf(TEXT("Cannot map a file with a length of zero.\n"));
     return FALSE;
  }

  // Create a file mapping object.
  hFileMap = CreateFileMapping(hFile, 
                    NULL, 
                    PAGE_READONLY,
                    0, 
                    1,
                    NULL);

  if (hFileMap) 
  {
    // Create a file mapping to get the file name.
    void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);

    if (pMem) 
    {
      if (GetMappedFileName (GetCurrentProcess(), 
                             pMem, 
                             pszFilename,
                             MAX_PATH)) 
      {

        // Translate path with device name to drive letters.
        TCHAR szTemp[BUFSIZE];
        szTemp[0] = '\0';

        if (GetLogicalDriveStrings(BUFSIZE-1, szTemp)) 
        {
          TCHAR szName[MAX_PATH];
          TCHAR szDrive[3] = TEXT(" :");
          BOOL bFound = FALSE;
          TCHAR* p = szTemp;

          do 
          {
            // Copy the drive letter to the template string
            *szDrive = *p;

            // Look up each device name
            if (QueryDosDevice(szDrive, szName, MAX_PATH))
            {
              size_t uNameLen = _tcslen(szName);

              if (uNameLen < MAX_PATH) 
              {
                bFound = _tcsnicmp(pszFilename, szName, uNameLen) == 0
                         && *(pszFilename + uNameLen) == _T('\\');

                if (bFound) 
                {
                  // Reconstruct pszFilename using szTempFile
                  // Replace device path with DOS path
                  TCHAR szTempFile[MAX_PATH];
                  StringCchPrintf(szTempFile,
                            MAX_PATH,
                            TEXT("%s%s"),
                            szDrive,
                            pszFilename+uNameLen);
                  StringCchCopyN(pszFilename, MAX_PATH+1, szTempFile, _tcslen(szTempFile));
                  const size_t len = _tcslen(pszFilename);
                  *d = len+1;
                }
              }
            }

            // Go to the next NULL character.
            while (*p++);
          } while (!bFound && *p); // end of string
        }
      }
      bSuccess = TRUE;
      UnmapViewOfFile(pMem);
    } 

    CloseHandle(hFileMap);
  }

  _tprintf(TEXT("File name is %s\n"), pszFilename);
  size_t len = _tcslen(pszFilename);
  if (fileName != NULL && !_tcslen(fileName) < len)
  {
	  _tcscpy_s(fileName, len + 1, (const TCHAR*)& pszFilename);
  }

  //fileName[len] = _T('\0');
  return(fileName);
}

int _tmain(int argc, TCHAR* argv[])
{
	HANDLE hFile;

	if (argc != 2)
	{
		_tprintf(TEXT("This sample takes a file name as a parameter.\n"));
		return 0;
	}
	hFile = CreateFile(argv[1], GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		_tprintf(TEXT("CreateFile failed with %d\n"), GetLastError());
		return 0;
	}

	size_t d;
	TCHAR* fileName = nullptr;
	
	GetFileNameFromHandle(hFile, &d, fileName);
	fileName = new TCHAR[d];
	GetFileNameFromHandle(hFile, &d, fileName);

	_tcslwr_s(argv[1], _tcslen(argv[1])+1);
	_tcslwr_s(fileName,_tcslen(fileName)+1);

	_tprintf(_T("File1: %s\n"), argv[1]);
	_tprintf(_T("File2: %s\n"), fileName);

    if(!_tcsicmp(argv[1], fileName))
    {
        _tprintf(_T("File is identical\n"));
    }
    else
    {
		_tprintf(_T("File is not identical\n"));
    }
    
    return 0;
}