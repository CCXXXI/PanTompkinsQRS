#pragma once

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void init(int fs);
bool panTompkins(float sample);

#ifdef __cplusplus
}  // extern "C"
#endif
