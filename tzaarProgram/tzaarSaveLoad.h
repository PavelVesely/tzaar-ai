/*
 * The header file for module tzaarSaveLoad
 * 
 * Author: Pavel Veselý
 * License: GPL v3, see license.txt
*/
#ifndef TZAARSAVELOAD_H_INCLUDED
#define TZAARSAVELOAD_H_INCLUDED

#include "main.h"

i32 SavePosition(const char *fileName);
i32 LoadPosition(const char *fileName);
i32 SaveBestMoves(const char *fileName, Move * m1, Move * m2);

#endif				// TZAARSAVELOAD_H_INCLUDED
