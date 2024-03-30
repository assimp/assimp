/*
 * Copyright (c) 2011 Apple Inc. All rights reserved.
 *
 * @APPLE_APACHE_LICENSE_HEADER_START@
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * @APPLE_APACHE_LICENSE_HEADER_END@
 */

//
//  EndianPortable.h
//
//  Copyright 2011 Apple Inc. All rights reserved.
//

#ifndef _EndianPortable_h
#define _EndianPortable_h

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint16_t Swap16NtoB(uint16_t inUInt16);
uint16_t Swap16BtoN(uint16_t inUInt16);

uint32_t Swap32NtoB(uint32_t inUInt32);
uint32_t Swap32BtoN(uint32_t inUInt32);

uint64_t Swap64BtoN(uint64_t inUInt64);
uint64_t Swap64NtoB(uint64_t inUInt64);

float SwapFloat32BtoN(float in);
float SwapFloat32NtoB(float in);

double SwapFloat64BtoN(double in);
double SwapFloat64NtoB(double in);

void Swap16(uint16_t * inUInt16);
void Swap24(uint8_t * inUInt24);
void Swap32(uint32_t * inUInt32);

#ifdef __cplusplus
}
#endif

#endif
