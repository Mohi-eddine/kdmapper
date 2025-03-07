/*
#####################
# MoHieDDiNNe Codes #
#####################
*/

#pragma once
#include <stdio.h>
#include <windows.h>

#define CURRENT_PROCESS (HANDLE)-1

#ifndef NDEBUG
#define Print(...) printf(__VA_ARGS__)
#else
#define Print(...)
#endif

