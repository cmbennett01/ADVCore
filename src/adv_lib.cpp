/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <cstdint>

#include "adv_lib.h"
#include "adv_image_layout.h"
#include "adv_profiling.h"
#include "adv2_file.h"


char* g_CurrentAdvFile;
AdvLib::AdvFile* g_AdvFile;
bool g_FileStarted = false;

AdvLib2::Adv2File* g_Adv2File;

using namespace std;

char* AdvGetCurrentFilePath(void)
{
	return g_CurrentAdvFile;
}

unsigned int AdvGetFileVersion(const char* fileName)
{
	FILE* probe = advfopen(fileName, "rb");
	if (probe == 0) return 0;
	
	unsigned int buffInt;
	unsigned char buffChar;

	advfread(&buffInt, 4, 1, probe);
	advfread(&buffChar, 1, 1, probe);
	advfclose(probe);	
	
	if (buffInt != 0x46545346) return 0;
	return (unsigned int)buffChar;
}

int AdvOpenFile(const char* fileName)
{
	AdvCloseFile();

	FILE* probe = advfopen(fileName, "rb");
	if (probe == 0) return 0;
	
	unsigned int buffInt;
	unsigned char buffChar;

	advfread(&buffInt, 4, 1, probe);
	advfread(&buffChar, 1, 1, probe);
	advfclose(probe);
	
	if (buffInt != 0x46545346) return 0;
	
	if (buffChar == 1)
	{
		if (nullptr != g_AdvFile)
		{
			delete g_AdvFile;
			g_AdvFile = nullptr;
		}
		
		g_FileStarted = false;
		
		int len = (int)strlen(fileName);
		if (len > 0)
		{
			g_CurrentAdvFile = new char[len + 1];
			strncpy_s(g_CurrentAdvFile, len + 1, fileName, len + 1);
		
			g_AdvFile = new AdvLib::AdvFile();
			int res = !g_AdvFile->LoadFile(fileName);
			if (res < 0)
			{
				delete g_AdvFile;
				g_AdvFile = nullptr;
				return res;
			}
		}
		
		return 1;
	}
	else if (buffChar == 2)
	{
		if (nullptr != g_Adv2File)
		{
			delete g_Adv2File;
			g_Adv2File = nullptr;
		}
		
		g_FileStarted = false;
		
		int len = (int)strlen(fileName);
		if (len > 0)
		{
			g_CurrentAdvFile = new char[len + 1];
			strncpy_s(g_CurrentAdvFile, len + 1, fileName, len + 1);
		
			g_Adv2File = new AdvLib2::Adv2File();
			int res = !g_Adv2File->LoadFile(fileName);
			if (res < 0)
			{
				delete g_Adv2File;
				g_Adv2File = nullptr;
				return res;
			}
		}
		
		return 2;
	}

	return 0;
}

unsigned int AdvCloseFile()
{
	unsigned int rv = 0;

	if (nullptr != g_AdvFile)
	{
		g_AdvFile->CloseFile();
		delete g_AdvFile;
		g_AdvFile = nullptr;
		rv += 1;
	}

	if (nullptr != g_Adv2File)
	{
		g_Adv2File->CloseFile();
		delete g_Adv2File;
		g_Adv2File = nullptr;
		rv += 2;
	}

	if (nullptr != g_CurrentAdvFile)
	{
		delete g_CurrentAdvFile;
		g_CurrentAdvFile = nullptr;
	}

	return rv;
}

void AdvVer1_NewFile(const char* fileName)
{
	AdvProfiling_ResetPerformanceCounters();
	AdvProfiling_StartProcessing();
	
    if (nullptr != g_AdvFile)
	{
		delete g_AdvFile;
		g_AdvFile = nullptr;		
	}
	
	if (nullptr != g_CurrentAdvFile)
	{
		delete g_CurrentAdvFile;
		g_CurrentAdvFile = nullptr;
	}
	
	g_FileStarted = false;
	
	int len = (int)strlen(fileName);	
	if (len > 0)
	{
		g_CurrentAdvFile = new char[len + 1];
		strncpy_s(g_CurrentAdvFile, len + 1, fileName, len + 1);
	
		g_AdvFile = new AdvLib::AdvFile();	
	}
	AdvProfiling_EndProcessing();
}

void AdvVer1_EndFile()
{
	if (nullptr != g_AdvFile)
	{
		g_AdvFile->EndFile();
		
		delete g_AdvFile;
		g_AdvFile = nullptr;		
	}
	
	if (nullptr != g_CurrentAdvFile)
	{
		delete g_CurrentAdvFile;
		g_CurrentAdvFile = nullptr;
	}
	
	g_FileStarted = false;
}

void AdvVer1_DefineImageSection(unsigned short width, unsigned short height, unsigned char dataBpp)
{
	AdvProfiling_StartProcessing();
	AdvLib::AdvImageSection* imageSection = new AdvLib::AdvImageSection(width, height, dataBpp);
	g_AdvFile->AddImageSection(imageSection);
	AdvProfiling_EndProcessing();
}

void AdvVer1_DefineImageLayout(unsigned char layoutId, const char* layoutType, const char* compression, unsigned char bpp, int keyFrame, const char* diffCorrFromBaseFrame)
{	
	AdvProfiling_StartProcessing();
	AdvLib::AdvImageLayout* imageLayout = g_AdvFile->ImageSection->AddImageLayout(layoutId, layoutType, compression, bpp, keyFrame);
	if (diffCorrFromBaseFrame != nullptr)
		imageLayout->AddOrUpdateTag("DIFFCODE-BASE-FRAME", diffCorrFromBaseFrame);
		
	AdvProfiling_EndProcessing();
}

unsigned int AdvVer1_DefineStatusSectionTag(const char* tagName, int tagType)
{
	AdvProfiling_StartProcessing();
	unsigned int statusTagId = g_AdvFile->StatusSection->DefineTag(tagName, (AdvTagType)tagType);
	AdvProfiling_EndProcessing();
	return statusTagId;
}

unsigned int AdvVer1_AddFileTag(const char* tagName, const char* tagValue)
{
	AdvProfiling_StartProcessing();	
	unsigned int fileTagId = g_AdvFile->AddFileTag(tagName, tagValue);
	AdvProfiling_EndProcessing();
	return fileTagId;
}

void AdvVer1_AddOrUpdateImageSectionTag(const char* tagName, const char* tagValue)
{
	AdvProfiling_StartProcessing();
	return g_AdvFile->ImageSection->AddOrUpdateTag(tagName, tagValue);
	AdvProfiling_EndProcessing();
}

bool AdvVer1_BeginFrame(__int64 timeStamp, unsigned int elapsedTime, unsigned int exposure)
{
	AdvProfiling_StartProcessing();
	if (!g_FileStarted)
	{
		bool success = g_AdvFile->BeginFile(g_CurrentAdvFile);
		if (success)
		{
			g_FileStarted = true;	
		}
		else
		{
			g_FileStarted = false;
			return false;
		}		
	}
	
	g_AdvFile->BeginFrame(timeStamp, elapsedTime, exposure);
	AdvProfiling_EndProcessing();
	return true;
}

void AdvVer1_FrameAddImageBytes(unsigned char layoutId,  unsigned char* pixels, unsigned char pixelsBpp)
{
	AdvProfiling_StartProcessing();
	g_AdvFile->AddFrameImage(layoutId, (unsigned short*)pixels, pixelsBpp);
	AdvProfiling_EndProcessing();
}

void AdvVer1_FrameAddImage(unsigned char layoutId,  unsigned short* pixels, unsigned char pixelsBpp)
{
	AdvProfiling_StartProcessing();
	g_AdvFile->AddFrameImage(layoutId, pixels, pixelsBpp);
	AdvProfiling_EndProcessing();
}

void AdvVer1_FrameAddStatusTag(unsigned int tagIndex, const char* tagValue)
{
	AdvProfiling_StartProcessing();
	g_AdvFile->AddFrameStatusTag(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer1_FrameAddStatusTagMessage(unsigned int tagIndex, const char* tagValue)
{	
	AdvProfiling_StartProcessing();
	g_AdvFile->AddFrameStatusTagMessage(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer1_FrameAddStatusTagUInt8(unsigned int tagIndex, unsigned char tagValue)
{
	AdvProfiling_StartProcessing();
	g_AdvFile->AddFrameStatusTagUInt8(tagIndex, tagValue);
	AdvProfiling_EndProcessing();	
}

void AdvVer1_FrameAddStatusTag32(unsigned int tagIndex, unsigned int tagValue)
{
	AdvProfiling_StartProcessing();
	g_AdvFile->AddFrameStatusTagUInt32(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer1_FrameAddStatusTag64(unsigned int tagIndex, __int64 tagValue)
{
	AdvProfiling_StartProcessing();
	g_AdvFile->AddFrameStatusTagUInt64(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer1_FrameAddStatusTag16(unsigned int tagIndex, unsigned short tagValue)
{
	AdvProfiling_StartProcessing();
	g_AdvFile->AddFrameStatusTagUInt16(tagIndex, tagValue);
	AdvProfiling_EndProcessing();	
}

void AdvVer1_FrameAddStatusTagReal(unsigned int tagIndex, float tagValue)
{
	AdvProfiling_StartProcessing();
	g_AdvFile->AddFrameStatusTagReal(tagIndex, tagValue);
	AdvProfiling_EndProcessing();	
}

void AdvVer1_EndFrame()
{
	AdvProfiling_StartProcessing();
	g_AdvFile->EndFrame();
	AdvProfiling_EndProcessing();
}

void GetLibraryVersion(char* version)
{
	strcpy_s(version, strlen(CORE_VERSION) + 1, CORE_VERSION);
}

int GetLibraryBitness()
{
	#if __GNUC__
	
		#if defined(_WIN64)
			// Windows compilation with GCC
			return 64;
		#elif defined(_WIN32)
			// Windows compilation with GCC
			return 32;
		#endif
		
		// Linux/OSX Compilation
		
		// All modern 64-bit Unix systems use LP64. MacOS X and Linux are both modern 64-bit systems.
		//	Type           ILP64   LP64   LLP64
		//  char              8      8       8
		//  short            16     16      16
		//  int              64     32      32
		//  long             64     64      32
		//  long long        64     64      64
		//  pointer          64     64      64
		//------------------------------------
		// On a Unix system (gcc/g++ compiler) the bitness can be determined by the size of 'long'
		return sizeof(long) * 8;
		
	#endif
	#if _WIN32 || _WIN64

		#if defined(_WIN64)
			return 64;  // 64-bit programs run only on Win64
			
		#elif defined(_WIN32)
			//// 32-bit programs run on both 32-bit and 64-bit Windows so must sniff
			//BOOL f64 = FALSE;
			//if (IsWow64Process(GetCurrentProcess(), &f64) && f64)
			//	return 64;
			//else
			//	return 32;

			// We only care if the binary is 32 or 64 bit, so ignore the IsWow64Process thing
			return 32;
		#else
			return 16; // Win64 does not support Win16
		#endif
	#endif
}

void GetLibraryPlatformId(char* platform)
{
#define PLATFORM_WIN_MSVC_32 "MS VC++, x86, Windows"
#define PLATFORM_WIN_MSVC_64 "MS VC++, AMD64, Windows"
#define PLATFORM_WIN_GNU_32 "GNU GCC/G++, x86, Windows"
#define PLATFORM_WIN_GNU_64 "GNU GCC/G++, AMD64, Windows"
#define PLATFORM_LINUX_GNU "GNU GCC/G++, Linux"
#define PLATFORM_OSX_GNU "GNU GCC/G++, OSX"
#define PLATFORM_UNKNOWN "Unknown"

#ifdef MSVC
	#if INTPTR_MAX == INT32_MAX
		strcpy_s(platform, strlen(PLATFORM_WIN_MSVC_32) + 1, PLATFORM_WIN_MSVC_32);
	#elif INTPTR_MAX == INT64_MAX
		strcpy_s(platform, strlen(PLATFORM_WIN_MSVC_64) + 1, PLATFORM_WIN_MSVC_64);
	#endif
#elif __linux__
	strcpy_s(platform, strlen(PLATFORM_LINUX_GNU) + 1, PLATFORM_LINUX_GNU);
#elif __APPLE__
	strcpy_s(platform, strlen(PLATFORM_OSX_GNU) + 1, PLATFORM_OSX_GNU);
#elif __GNUC__ || __GNUG__
	#if __x86_64__ || __ppc64__ || _WIN64
		strcpy_s(platform, strlen(PLATFORM_WIN_GNU_64) + 1, PLATFORM_WIN_GNU_64);
	#else
		strcpy_s(platform, strlen(PLATFORM_WIN_GNU_32) + 1, PLATFORM_WIN_GNU_32);
	#endif	
#else
	strcpy_s(platform, strlen(PLATFORM_UNKNOWN) + 1, PLATFORM_UNKNOWN);
#endif
}

void AdvVer2_NewFile(const char* fileName)
{
	AdvProfiling_ResetPerformanceCounters();
	AdvProfiling_StartProcessing();
	
    if (nullptr != g_Adv2File)
	{
		delete g_Adv2File;
		g_Adv2File = nullptr;
	}
	
	if (nullptr != g_CurrentAdvFile)
	{
		delete g_CurrentAdvFile;
		g_CurrentAdvFile = nullptr;
	}
	
	g_FileStarted = false;
	
	int len = (int)strlen(fileName);
	if (len > 0)
	{
		g_CurrentAdvFile = new char[len + 1];
		strncpy_s(g_CurrentAdvFile, len + 1, fileName, len + 1);
		
		g_Adv2File = new AdvLib2::Adv2File();
	}
	AdvProfiling_EndProcessing();
}

void AdvVer2_SetTicksTimingPrecision(int mainStreamAccuracy, int calibrationStreamAccuracy)
{
	if (nullptr != g_Adv2File)
	{
		g_Adv2File->SetTicksTimingPrecision(mainStreamAccuracy, calibrationStreamAccuracy);
	}
}

void AdvVer2_DefineExternalClockForMainStream(__int64 clockFrequency, int ticksTimingAccuracy)
{
	if (nullptr != g_Adv2File)
	{
		g_Adv2File->DefineExternalClockForMainStream(clockFrequency, ticksTimingAccuracy);
	}
}

void AdvVer2_DefineExternalClockForCalibrationStream(__int64 clockFrequency, int ticksTimingAccuracy)
{
	if (nullptr != g_Adv2File)
	{
		g_Adv2File->DefineExternalClockForCalibrationStream(clockFrequency, ticksTimingAccuracy);
	}
}


void AdvVer2_EndFile()
{
	if (nullptr != g_Adv2File)
	{
		g_Adv2File->EndFile();
		
		delete g_Adv2File;
		g_Adv2File = nullptr;
	}
	
	if (nullptr != g_CurrentAdvFile)
	{
		delete g_CurrentAdvFile;
		g_CurrentAdvFile = nullptr;
	}
	
	g_FileStarted = false;
}

unsigned int AdvVer2_AddMainStreamTag(const char* tagName, const char* tagValue)
{
	AdvProfiling_StartProcessing();
	int tagId = g_Adv2File->AddMainStreamTag(tagName, tagValue);
	AdvProfiling_EndProcessing();

	return tagId;
}

unsigned int AdvVer2_AddCalibrationStreamTag(const char* tagName, const char* tagValue)
{
	AdvProfiling_StartProcessing();
	int tagId = g_Adv2File->AddCalibrationStreamTag(tagName, tagValue);
	AdvProfiling_EndProcessing();

	return tagId;
}

bool AdvVer2_BeginFrame(unsigned int streamId, __int64 utcStartTimeNanosecondsSinceAdvZeroEpoch, unsigned int utcExposureNanoseconds)
{
	AdvProfiling_StartProcessing();
	if (!g_FileStarted)
	{
		bool success = g_Adv2File->BeginFile(g_CurrentAdvFile);
		if (success)
		{
			g_FileStarted = true;
		}
		else
		{
			g_FileStarted = false;
			return false;
		}		
	}
	
	g_Adv2File->BeginFrame(streamId, utcStartTimeNanosecondsSinceAdvZeroEpoch, utcExposureNanoseconds);
	AdvProfiling_EndProcessing();
	return true;
}

bool AdvVer2_BeginFrameWithTicks(unsigned int streamId, __int64 startFrameTicks, __int64 endFrameTicks, __int64 elapsedTicksSinceFirstFrame, __int64 utcStartTimeNanosecondsSinceAdvZeroEpoch, unsigned int utcExposureNanoseconds)
{
	AdvProfiling_StartProcessing();
	if (!g_FileStarted)
	{
		bool success = g_Adv2File->BeginFile(g_CurrentAdvFile);
		if (success)
		{
			g_FileStarted = true;	
		}
		else
		{
			g_FileStarted = false;
			return false;
		}		
	}
	
	g_Adv2File->BeginFrame(streamId, startFrameTicks, endFrameTicks, elapsedTicksSinceFirstFrame, utcStartTimeNanosecondsSinceAdvZeroEpoch, utcExposureNanoseconds);
	AdvProfiling_EndProcessing();
	return true;
}

void AdvVer2_EndFrame()
{
	AdvProfiling_StartProcessing();
	g_Adv2File->EndFrame();
	AdvProfiling_EndProcessing();
}

void AdvVer2_DefineImageSection(unsigned short width, unsigned short height, unsigned char dataBpp)
{
	AdvProfiling_StartProcessing();
	AdvLib2::Adv2ImageSection* imageSection = new AdvLib2::Adv2ImageSection(width, height, dataBpp);
	g_Adv2File->AddImageSection(imageSection);
	AdvProfiling_EndProcessing();
}

void AdvVer2_DefineStatusSection(__int64 utcTimestampAccuracyInNanoseconds)
{
	AdvProfiling_StartProcessing();
	AdvLib2::Adv2StatusSection* statusSection = new AdvLib2::Adv2StatusSection(utcTimestampAccuracyInNanoseconds);
	g_Adv2File->AddStatusSection(statusSection);
	AdvProfiling_EndProcessing();
}

void AdvVer2_DefineImageLayout(unsigned char layoutId, const char* layoutType, const char* compression, unsigned char layoutBpp)
{
	AdvProfiling_StartProcessing();
	AdvLib2::Adv2ImageLayout* imageLayout = g_Adv2File->ImageSection->AddImageLayout(layoutId, layoutType, compression, layoutBpp);		
	AdvProfiling_EndProcessing();
}

unsigned int AdvVer2_DefineStatusSectionTag(const char* tagName, int tagType)
{
	AdvProfiling_StartProcessing();
	unsigned int statusTagId = g_Adv2File->StatusSection->DefineTag(tagName, (AdvTagType)tagType);
	AdvProfiling_EndProcessing();
	return statusTagId;
}

unsigned int AdvVer2_AddFileTag(const char* tagName, const char* tagValue)
{
	AdvProfiling_StartProcessing();
	unsigned int fileTagId = g_Adv2File->AddFileTag(tagName, tagValue);
	AdvProfiling_EndProcessing();
	return fileTagId;
}

void AdvVer2_AddOrUpdateImageSectionTag(const char* tagName, const char* tagValue)
{
	AdvProfiling_StartProcessing();
	return g_Adv2File->ImageSection->AddOrUpdateTag(tagName, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer2_FrameAddStatusTagUTF8String(unsigned int tagIndex, const char* tagValue)
{
	AdvProfiling_StartProcessing();
	g_Adv2File->AddFrameStatusTagUTF8String(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer2_FrameAddStatusTagMessage(unsigned int tagIndex, const char* tagValue)
{
	AdvProfiling_StartProcessing();
	g_Adv2File->AddFrameStatusTagMessage(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer2_FrameAddStatusTagUInt8(unsigned int tagIndex, unsigned char tagValue)
{
	AdvProfiling_StartProcessing();
	g_Adv2File->AddFrameStatusTagUInt8(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer2_FrameAddStatusTag16(unsigned int tagIndex, unsigned short tagValue)
{
	AdvProfiling_StartProcessing();
	g_Adv2File->AddFrameStatusTagUInt16(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer2_FrameAddStatusTagReal(unsigned int tagIndex, float tagValue)
{
	AdvProfiling_StartProcessing();
	g_Adv2File->AddFrameStatusTagReal(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer2_FrameAddStatusTag32(unsigned int tagIndex, unsigned int tagValue)
{
	AdvProfiling_StartProcessing();
	g_Adv2File->AddFrameStatusTagUInt32(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer2_FrameAddStatusTag64(unsigned int tagIndex, __int64 tagValue)
{
	AdvProfiling_StartProcessing();
	g_Adv2File->AddFrameStatusTagUInt64(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

/* Assumed pixel format by AdvCore when this method is called

    |    Layout Type    |  ImageLayout.Bpp |  Assumed Pixel Format                                         |
    |  FULL-IMAGE-RAW   |    16, 12, 8     | 16-bit data (1 short per pixel)                               |
    |12BIT-IMAGE-PACKED |    12            | 16-bit data (1 short per pixel) will be packed when storing   |
    
	All other combinations which are not listed above are invalid.
*/
HRESULT AdvVer2_FrameAddImage(unsigned char layoutId, unsigned short* pixels, unsigned char pixelsBpp)
{
	AdvProfiling_StartProcessing();
	HRESULT rv = g_Adv2File->AddFrameImage(layoutId, pixels, pixelsBpp);
	AdvProfiling_EndProcessing();
	return rv;
}

/* Assumed pixel format by AdvCore when this method is called

    |    Layout Type    |  ImageLayout.Bpp |  Assumed Pixel Format                                         |
    |  FULL-IMAGE-RAW   |    16, 12        | 16-bit little endian data passed as bytes (2 bytes per pixel) |
	|  FULL-IMAGE-RAW   |     8            | 8-bit data passed as bytes (1 byte per pixel)                 |
    |12BIT-IMAGE-PACKED |    12            | 12-bit packed data (3 bytes per 2 pixels)                     |
    | 8BIT-COLOR-IMAGE  |     8            | 8-bit RGB or BGR data (3 bytes per pixel, 1 colour per byte)  |

	All other combinations which are not listed above are invalid.
*/
HRESULT AdvVer2_FrameAddImageBytes(unsigned char layoutId, unsigned char* pixels, unsigned char pixelsBpp)
{
	AdvProfiling_StartProcessing();
	HRESULT rv = g_Adv2File->AddFrameImage(layoutId, pixels, pixelsBpp);
	AdvProfiling_EndProcessing();
	return rv;
}

void AdvVer2_GetMainStreamInfo(int* numFrames, __int64* mainClockFrequency, int* mainStreamAccuracy)
{
	g_Adv2File->GetMainStreamInfo(numFrames, mainClockFrequency, mainStreamAccuracy);
}

void AdvVer2_GetCalibrationStreamInfo(int* numFrames, __int64* calibrationClockFrequency, int* calibrationStreamAccuracy)
{
	g_Adv2File->GetCalibrationStreamInfo(numFrames, calibrationClockFrequency, calibrationStreamAccuracy);
}

HRESULT AdvVer2_GetFramePixels(int streamId, int frameNo, unsigned int* pixels, AdvLib2::AdvFrameInfo* frameInfo, char* systemError)
{
	if (streamId == 0 && frameNo >= g_Adv2File->TotalNumberOfMainFrames)
		return E_FAIL;

	if (streamId > 0 && frameNo >= g_Adv2File->TotalNumberOfCalibrationFrames)
		return E_FAIL;

        unsigned char layoutId;
        enum GetByteMode byteMode;

        g_Adv2File->GetFrameImageSectionHeader(streamId, frameNo, &layoutId, &byteMode);

		AdvLib2::Adv2ImageLayout* layout = g_Adv2File->ImageSection->GetImageLayoutById(layoutId);
		
        g_Adv2File->GetFrameSectionData(streamId, frameNo, pixels, frameInfo, systemError);
	
		return S_OK;
}

