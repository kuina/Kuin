// Cipher with AES-256.

#pragma once

#include "..\common.h"

EXPORT void* _encrypt(const U8* data, const U8* key);
EXPORT void* _decrypt(const U8* data, const U8* key);
