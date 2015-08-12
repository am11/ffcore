#include "pch.h"
#include "App/Log.h"
#include "Data/DataFile.h"
#include "Data/DataWriterReader.h"
#include "Globals/ProcessGlobals.h"
#include "Windows/FileUtil.h"

ff::Log::Log()
{
}

ff::Log::~Log()
{
}

ff::IDataWriter *ff::Log::AddFile(StringRef path, bool bAppend)
{
	ComPtr<IDataFile>   pFile;
	ComPtr<IDataWriter> pWriter;

	assertRetVal(CreateDataFile(path, false, &pFile), nullptr);
	assertRetVal(CreateDataWriter(pFile, bAppend ? pFile->GetSize() : INVALID_SIZE, &pWriter), nullptr);

	if (!bAppend || !pFile->GetSize())
	{
		assertRetVal(WriteUnicodeBOM(pWriter), nullptr);
	}

	assertRetVal(AddWriter(pWriter), nullptr);

	return pWriter;
}

bool ff::Log::AddWriter(IDataWriter *pWriter)
{
	LockMutex crit(_cs);

	assertRetVal(_writers.Find(pWriter) == INVALID_SIZE, false);
	_writers.Push(pWriter);

	return true;
}

bool ff::Log::RemoveWriter(IDataWriter *writer)
{
	LockMutex crit(_cs);

	assertRetVal(writer, false);

	size_t i = _writers.Find(writer);
	assertRetVal(i != INVALID_SIZE, false);

	_writers.Delete(i);

	return true;
}

void ff::Log::RemoveAllWriters()
{
	LockMutex crit(_cs);

	_writers.Clear();
}

void ff::Log::Trace(const wchar_t *szText)
{
	assertRet(szText);

	if (!_writers.IsEmpty())
	{
		LockMutex crit(_cs);

		if (!_writers.IsEmpty())
		{
			DWORD nBytes = (DWORD)(wcslen(szText) * sizeof(wchar_t));

			for (IDataWriter *writer: _writers)
			{
				writer->Write(szText, nBytes);
			}
		}
	}

	DebugTrace(szText);
}

void ff::Log::TraceF(const wchar_t *szFormat, ...)
{
	va_list args;
	va_start(args, szFormat);
	TraceV(szFormat, args);
	va_end(args);
}

void ff::Log::TraceV(const wchar_t *szFormat, va_list args)
{
	wchar_t sz[1024];
	_vsnwprintf_s(sz, _countof(sz), _TRUNCATE, szFormat, args);
	Trace(sz);
}

void ff::Log::TraceLine(const wchar_t *szText)
{
	String finalText(szText ? szText : L"");
	finalText += L"\r\n";
	Trace(finalText.c_str());
}

void ff::Log::TraceLineF(const wchar_t *szFormat, ...)
{
	va_list args;
	va_start(args, szFormat);
	TraceLineV(szFormat, args);
	va_end(args);
}

void ff::Log::TraceLineV(const wchar_t *szFormat, va_list args)
{
	wchar_t sz[1024];
	_vsnwprintf_s(sz, _countof(sz), _TRUNCATE, szFormat, args);
	TraceLine(sz);
}

void ff::Log::TraceSpaces(size_t nSpaces)
{
	wchar_t sz[128];
	_snwprintf_s(sz, _countof(sz), _TRUNCATE, L"%*s", (int)nSpaces, L"");
	Trace(sz);
}

// static
void ff::Log::GlobalTrace(const wchar_t *szText)
{
	ProcessGlobals *globals = ProcessGlobals::Get();
	noAssertRet(globals);
	globals->GetLog().Trace(szText);
}

// static
void ff::Log::GlobalTraceF(const wchar_t *szFormat, ...)
{
	va_list args;
	va_start(args, szFormat);
	GlobalTraceV(szFormat, args);
	va_end(args);
}

// static
void ff::Log::GlobalTraceV(const wchar_t *szFormat, va_list args)
{
	ProcessGlobals *globals = ProcessGlobals::Get();
	noAssertRet(globals);
	globals->GetLog().TraceV(szFormat, args);
}


// static
void ff::Log::DebugTrace(const wchar_t *szText)
{
#ifdef _DEBUG
	assertRet(szText);
	LockMutex crit(GCS_LOG);
	::OutputDebugStringW(szText);
#endif
}

// static
void ff::Log::DebugTraceF(const wchar_t *szFormat, ...)
{
#ifdef _DEBUG
	va_list args;
	va_start(args, szFormat);
	DebugTraceV(szFormat, args);
	va_end(args);
#endif
}

// static
void ff::Log::DebugTraceV(const wchar_t *szFormat, va_list args)
{
#ifdef _DEBUG
	std::array<wchar_t, 1024> buffer;
	_vsnwprintf_s(buffer.data(), buffer.size(), _TRUNCATE, szFormat, args);
	DebugTrace(buffer.data());
#endif
}
