#include <iostream>
#include <fstream>
#include <algorithm>

#include <Windows.h>
#include <shlwapi.h>
#include <cassert>
#include <atlconv.h>

#include "7z/7z.h"
#include "7z/7zCrc.h"
#include "7z/7zFile.h"
#include "7z/7zVersion.h"

extern "C"
{
#include "7z/7zAlloc.h"
}

//Windows路径目录操作MSDN参考
//路径操作: https://msdn.microsoft.com/en-us/library/windows/desktop/bb773559(v=vs.85).aspx
//目录操作: https://msdn.microsoft.com/en-us/library/windows/desktop/aa363950(v=vs.85).aspx

static BOOL CreateDirectoryRecur(PCWSTR szFullPath)
{
	//创建路径szFullPath, 如果父目录不存在,同时创建父目录
	assert (szFullPath && szFullPath[wcslen(szFullPath) - 1] != L'/' && szFullPath[wcslen(szFullPath) - 1] != L'\\');
	BOOL bCreate = CreateDirectoryW(szFullPath, NULL);
	if (!bCreate)
	{
		DWORD dErr = GetLastError();
		if (dErr == ERROR_PATH_NOT_FOUND)
		{
			WCHAR szParentPath[MAX_PATH] = {0};
			memcpy(szParentPath, szFullPath, wcslen(szFullPath) * sizeof(WCHAR));
			::PathRemoveFileSpecW(szParentPath);
			bCreate = CreateDirectoryW(szParentPath, NULL);
			return bCreate ? CreateDirectoryW(szFullPath, NULL) : bCreate;
		}
		else if (dErr = ERROR_ALREADY_EXISTS)
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	return bCreate;
};

bool Unzip7zip(PCWSTR szZipFile, PCWSTR szDestDir)
{
	if (NULL == szZipFile || NULL == szDestDir)
		return false;
	if (!::PathFileExistsW(szZipFile) || !::PathFileExistsW(szDestDir))
		return false;
	
	USES_CONVERSION;

	CFileInStream archiveStream;
	CLookToRead lookStream;
	CSzArEx db;
	SRes res;
	ISzAlloc allocImp;
	ISzAlloc allocTempImp;

	allocImp.Alloc = SzAlloc;
	allocImp.Free = SzFree;

	allocTempImp.Alloc = SzAllocTemp;
	allocTempImp.Free = SzFreeTemp;

	if (InFile_OpenW(&archiveStream.file, szZipFile))
	{
		return false;
	}

	FileInStream_CreateVTable(&archiveStream);
	LookToRead_CreateVTable(&lookStream, False);

	lookStream.realStream = &archiveStream.s;
	LookToRead_Init(&lookStream);

	CrcGenerateTable();

	SzArEx_Init(&db);
	res=SzArEx_Open(&db, &lookStream.s, &allocImp, &allocTempImp);

	if(res==SZ_OK)
	{
		UInt32 i;

		/*
		if you need cache, use these 3 variables.
		if you use external function, you can make these variable as static.
		*/
		UInt32 blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
		Byte *outBuffer = 0; /* it must be 0 before first call for each new archive. */
		size_t outBufferSize = 0;  /* it can have any value before first call (if outBuffer = 0) */
		size_t progressSize = 0;
		std::wstring fileName;

		for (i = 0; i < db.db.NumFiles; i++)
		{
			size_t offset = 0;
			size_t outSizeProcessed = 0;
			const CSzFileItem *f = db.db.Files + i;
			size_t len;
			len = SzArEx_GetFileNameUtf16(&db, i, NULL);
			fileName.resize(len + 1);
			SzArEx_GetFileNameUtf16(&db, i, (UInt16*)fileName.data());
			std::transform(fileName.begin(), fileName.end(), fileName.begin(), [](WCHAR wchr) { return wchr == L'/' ? L'\\' : wchr; });
			if (f->IsDir)
			{
				if (!fileName.empty())
				{
					WCHAR szDirFullPath[MAX_PATH] = {0};
					::PathCombineW(szDirFullPath, szDestDir, fileName.c_str());
					BOOL bSuccess = ::CreateDirectoryRecur(szDirFullPath);
					assert(bSuccess);
				}
				continue;
			}
			else
			{
				WCHAR szFileAbsFullName[MAX_PATH] = {0};
				::PathCombineW(szFileAbsFullName, szDestDir, fileName.c_str());
				WCHAR szFileAbsFullPath[MAX_PATH] = {0};
				::memcpy(szFileAbsFullPath, szFileAbsFullName, sizeof(szFileAbsFullName));
				::PathRemoveFileSpecW(szFileAbsFullPath);
				BOOL bSuccess = ::CreateDirectoryRecur(szFileAbsFullPath);
				assert(bSuccess);

				res = SzArEx_Extract(&db, &lookStream.s, i,
					&blockIndex, &outBuffer, &outBufferSize,
					&offset, &outSizeProcessed,
					&allocImp, &allocTempImp);
				if (res != SZ_OK)
					break;

				std::fstream fssNewFile(W2A(szFileAbsFullName), std::ios::out | std::ios::binary);
				if (!fssNewFile.good())
				{
					res = SZ_ERROR_FAIL;
					break;
				}
				fssNewFile.write((const char*)outBuffer+offset, outSizeProcessed);
				if (!fssNewFile.good())
				{
					res = SZ_ERROR_FAIL;
					break;
				}
				fssNewFile.close();
#ifdef USE_WINDOWS_FILE
				if (f->AttribDefined)
					SetFileAttributesW(szFileAbsFullName, f->Attrib);
#endif
				progressSize+=outSizeProcessed;
			}
		}
		IAlloc_Free(&allocImp, outBuffer);
		SzArEx_Free(&db, &allocImp);
	}

	File_Close(&archiveStream.file);
	return res==SZ_OK;
}

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		std::cout << "Usage: 7zipSdkDemo sourcezipFile unzipDir" << std::endl;
		return -1;
	}
	USES_CONVERSION;
	Unzip7zip(A2W(argv[1]), A2W(argv[2]));
	return 0;
}