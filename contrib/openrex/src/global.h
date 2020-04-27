/*
 * Copyright 2018 Robotic Eyes GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.*
 */
#pragma once

#define REX_FILE_MAGIC                  "REX1"
#define REX_FILE_VERSION                1

#define REX_BLOCK_HEADER_SIZE           16
#define REX_MESH_HEADER_SIZE            128
#define REX_MATERIAL_STANDARD_SIZE      68
#define REX_MESH_NAME_MAX_SIZE          74
#define REX_VERTEX_SIZE                 11

#define REX_NOT_SET                     0x7fffffffffffffffL
#define REX_EPSILON_FLOAT               0.000001f
