#pragma once

// Headers for CppUnitTest
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// Use stdafx.h from emucore
#include "../rpcs3/stdafx.h"

#include <locale>
#include <codecvt>

#define TEST_LOG(text, ...) Microsoft::VisualStudio::CppUnitTestFramework::Logger::WriteMessage(fmt::format("%s (line %d): " text, __FUNCTION__, __LINE__, __VA_ARGS__).c_str())
#define TEST_FAILURE(text, ...) Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(fmt::format(text, __VA_ARGS__)).c_str(), LINE_INFO())

// Emulator environment
#include "Emu/System.h"
#include "Emu/Memory/Memory.h"

static void setup_ps3_environment()
{
	vm::ps3::init();
}
