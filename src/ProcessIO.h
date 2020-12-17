#pragma once
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct GdbMsg
    {
        uint32_t    m_MsgSz;
        const char* m_Msg;
    } GdbMsg;

    //-----------------------------------------------------------------------------

    uint64_t GetHighResTime(void);

    long int SecToNano(double seconds);

    double NanoToSec(uint64_t nanosecs);

    void TimedWait(double secs);

    //-----------------------------------------------------------------------------

    int* GetFtoGPipes(void);

    int* GetGtoFPipes(void);

    bool SendCommand(const char* fmt, ...);

    GdbMsg GdbOutput(void);

    //-----------------------------------------------------------------------------

	typedef struct FileInfo
	{
		uint32_t m_LastAccess;
		uint32_t m_LastEdit;
		uint32_t m_LastChange;
		uint32_t m_Sz;
		char     m_Name[512];

		char*    m_Contents;
		uint32_t m_ContentMaxSz;
	} FileInfo;

	bool GetFileInfo(const char* fname, FileInfo* f_info);

	bool ReadFile(const char* fname, FileInfo* f_info);

#ifdef __cplusplus
}
#endif
