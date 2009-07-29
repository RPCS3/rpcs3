#pragma once

#ifndef CRT_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define CRT_H_62B23520_7C8E_11DE_8A39_0800200C9A66


// for detecting memory leaks
#ifdef _DEBUG

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#endif // _DEBUG


#endif // CRT_H_62B23520_7C8E_11DE_8A39_0800200C9A66
