// WavcombineC.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>
#include <memory.h>
#include <string.h>


typedef unsigned short WORD;
typedef unsigned long DWORD;

struct RIFF_HEADER
{
	char szRiffID[4];  // 'R','I','F','F'
	DWORD dwRiffSize;
	char szRiffFormat[4]; // 'W','A','V','E'
};

struct WAVE_FORMAT
{
	WORD wFormatTag;
	WORD wChannels;
	DWORD dwSamplesPerSec;
	DWORD dwAvgBytesPerSec;
	WORD wBlockAlign;
	WORD wBitsPerSample;
	WORD wAddData;//附加项可选
};


struct FMT_BLOCK
{
	char  szFmtID[4]; // 'f','m','t',' '
	DWORD  dwFmtSize;
	WAVE_FORMAT wavFormat;
};

struct FACT_BLOCK
{
	char  szFactID[4]; // 'f','a','c','t'
	DWORD  dwFactSize;
	DWORD dwData;
};

struct DATA_BLOCK
{
	char szDataID[4]; // 'd','a','t','a'
	DWORD dwDataSize;
};

void DelArray(void* pData)
{
	if (NULL != pData)
	{
		delete []pData;
		pData = NULL;
	}
}

bool CombineWaveFile(int argc, char** argv, char *pOutFileName)
{
	if (argc <= 0 || 0 == strlen(pOutFileName))
	{
		return false;
	}
	char strTemp[1024]={0};
	RIFF_HEADER *pRiff_Header = new RIFF_HEADER[argc];
	FMT_BLOCK *pFmt_Block = new FMT_BLOCK[argc];
	FACT_BLOCK *pFact_Block = new FACT_BLOCK[argc];
	DATA_BLOCK *pData_Block = new DATA_BLOCK[argc];
	int *pFileHandle = new int[argc];
	for (int i = 0; i < argc; i++)
	{
		memset(pRiff_Header+i, 0, sizeof(RIFF_HEADER));
		memset(pFmt_Block+i, 0, sizeof(FMT_BLOCK));
		memset(pFact_Block+i, 0, sizeof(FACT_BLOCK));
		memset(pData_Block+i, 0, sizeof(DATA_BLOCK));
		memset(pFileHandle+i, 0, sizeof(int));
		*(pFileHandle +i)= (int)(fopen(argv[i], "rb"));
		if (NULL == (pFileHandle+i))
		{
			DelArray(pRiff_Header);
			DelArray(pFmt_Block);
			DelArray(pFact_Block);
			DelArray(pData_Block);
			DelArray(pFileHandle);
			return false;
		}
	}

	for (int i = 0; i< argc; i++)
	{
		int nRead = fread(pRiff_Header+i, 1, sizeof(RIFF_HEADER), (FILE*)(pFileHandle[i]));
		if (nRead != sizeof(RIFF_HEADER))
		{
			DelArray(pRiff_Header);
			DelArray(pFmt_Block);
			DelArray(pFact_Block);
			DelArray(pData_Block);
			DelArray(pFileHandle);
			return false;
		}

		nRead = fread(pFmt_Block+i, 1, 8, (FILE*)(*(pFileHandle+i)));
		if (8 != nRead)
		{
			DelArray(pRiff_Header);
			DelArray(pFmt_Block);
			DelArray(pFact_Block);
			DelArray(pData_Block);
			DelArray(pFileHandle);
			return false;
		}

		nRead = fread(&((pFmt_Block+i)->wavFormat), 1, (pFmt_Block+i)->dwFmtSize, (FILE*)(*(pFileHandle+i)));
		if ((pFmt_Block+i)->dwFmtSize != nRead)
		{
			DelArray(pRiff_Header);
			DelArray(pFmt_Block);
			DelArray(pFact_Block);
			DelArray(pData_Block);
			DelArray(pFileHandle);
			return false;
		}

		nRead = fread(pFact_Block+i, 1, 8, (FILE*)(*(pFileHandle+i)));
		if (8 != nRead)
		{
			DelArray(pRiff_Header);
			DelArray(pFmt_Block);
			DelArray(pFact_Block);
			DelArray(pData_Block);
			DelArray(pFileHandle);
			return false;
		}

		//判断是fact字段还是data字段
		if(0 == strncmp((pFact_Block+i)->szFactID, "fact", 4))
		{
			nRead = fread(&((pFact_Block+i)->dwData), 1, sizeof((pFact_Block+i)->dwData), (FILE*)(*(pFileHandle+i)));
			if (sizeof((pFact_Block+i)->dwData) != nRead)
			{
				DelArray(pRiff_Header);
				DelArray(pFmt_Block);
				DelArray(pFact_Block);
				DelArray(pData_Block);
				DelArray(pFileHandle);
				return false;
			}
			nRead = fread(pData_Block+i, 1, sizeof(DATA_BLOCK), (FILE*)(*(pFileHandle+i)));
			if (nRead != sizeof(DATA_BLOCK))
			{
				DelArray(pRiff_Header);
				DelArray(pFmt_Block);
				DelArray(pFact_Block);
				DelArray(pData_Block);
				DelArray(pFileHandle);
				return false;
			}
		}
		else if(0 == strncmp((pFact_Block+i)->szFactID, "data", 4))//如果没有fact段就判断是不是data段
		{
			memcpy(pData_Block+i, pFact_Block+i, sizeof(DATA_BLOCK));
			memset(pFact_Block+i, 0 , sizeof(FACT_BLOCK));
		}
		else
		{
			DelArray(pRiff_Header);
			DelArray(pFmt_Block);
			DelArray(pFact_Block);
			DelArray(pData_Block);
			DelArray(pFileHandle);
			return false;
		}
	}
	//开始合并
	for(int i = 1; i < argc; i++)
	{
		pRiff_Header[0].dwRiffSize += pData_Block[i].dwDataSize; 
		pData_Block[0].dwDataSize += pData_Block[i].dwDataSize;
	}
	FILE *pOutFile = fopen(pOutFileName, "wb");
	if (NULL == pOutFile)
	{
		DelArray(pRiff_Header);
		DelArray(pFmt_Block);
		DelArray(pFact_Block);
		DelArray(pData_Block);
		DelArray(pFileHandle);
		return false;
	}
	int nWrite = fwrite(&(pRiff_Header[0]), 1, sizeof(RIFF_HEADER), pOutFile);
	nWrite = fwrite(&(pFmt_Block[0]), 1, 8, pOutFile);
	nWrite =fwrite(&(pFmt_Block[0].wavFormat), 1, pFmt_Block[0].dwFmtSize, pOutFile);
	if (pFact_Block[0].dwFactSize)
	{
		nWrite = fwrite(&(pFact_Block[0]), 1, sizeof(FACT_BLOCK), pOutFile);
	}
	nWrite = fwrite(&(pData_Block[0]), 1, sizeof(DATA_BLOCK), pOutFile);
	for (int i=0; i<argc; i++)
	{
		memset(strTemp, 0, sizeof(strTemp));
		int nRet = fread(strTemp, 1, sizeof(strTemp)-1, (FILE*)(*(pFileHandle+i)));
		while(0 != nRet)
		{
			fwrite(strTemp, 1, nRet, pOutFile);
			nRet = fread(strTemp, 1, sizeof(strTemp)-1, (FILE*)(*(pFileHandle+i)));
		}
		fclose((FILE*)(*(pFileHandle+i)));
	}

	fclose(pOutFile);
	DelArray(pRiff_Header);
	DelArray(pFmt_Block);
	DelArray(pFact_Block);
	DelArray(pData_Block);
	DelArray(pFileHandle);
	delete argv;
	return 1;
}

int _tmain(int argc, _TCHAR* argv[])
{
	/*char strFile[2][256];
	memset(strFile[0], 0, 256);
	sprintf(strFile[0], "f:\\123.wav");
	sprintf(strFile[1], "f:\\456.wav");*/
//	sprintf(strFile[2], "f:\\3.wav");

	char** str = new char*[2];
	str[0] = new char[256];
	str[1]= new char[256];
	sprintf(str[0], "f:\\123.wav");
	sprintf(str[1], "f:\\456.wav");
	CombineWaveFile(2, str, "f:\\out.wav");
	return 0;
}



