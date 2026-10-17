# Assimp MCP Server

![Assimp Logo](https://github.com/assimp/assimp/raw/master/contrib/assimp_logo.png)

**MCP Server for Open Asset Import Library (Assimp)**

This server provides a [Model Context Protocol](https://modelcontextprotocol.io/) interface to the Assimp command-line tool, enabling programmatic access to 3D model import/export functionality.

## Overview

Assimp (Open Asset Import Library) is a portable Open Source library to import various well-known 3D model formats in a uniform manner. This MCP server wraps the Assimp CLI tool (`assimp_cmd`) to provide its functionality through the MCP protocol.

## Features

- **Format Support**: Access 40+ 3D file formats through a unified interface
- **Import/Export**: Convert between various 3D model formats
- **Model Analysis**: Get detailed information about 3D models
- **Texture Extraction**: Extract embedded textures from 3D files
- **Batch Processing**: Test and process multiple files efficiently
- **Resource Access**: Query supported formats and version information

## Prerequisites

- Python 3.8+
- Assimp CLI tool (`assimp` or `assimp_cmd` executable)
- MCP-compatible client or runtime

## Installation

### From Source

```bash
# Clone the Assimp repository
 git clone https://github.com/assimp/assimp.git
 cd assimp

# Build Assimp (includes CLI tool)
cmake -G Ninja -DASSIMP_BUILD_ASSIMP_TOOLS=ON -B build
cmake --build build

# Install MCP server dependencies
cd tools/mcp_server
pip install -r requirements.txt
```

### As a Package

```bash
pip install git+https://github.com/assimp/assimp.git#subdirectory=tools/mcp_server
```

## Usage

### Starting the Server

```bash
# Method 1: Direct Python execution
python -m mcp_server.server

# Method 2: Using the installed script
assimp-mcp-server

# Method 3: With custom Assimp path
ASSIMP_CMD_PATH=/path/to/assimp python -m mcp_server.server
```

### With MCP Clients

#### Using Cursor (AI Code Editor)

Add to your Cursor settings:
```json
{
  "mcp": {
    "servers": {
      "assimp": {
        "command": "python",
        "args": ["-m", "mcp_server.server"],
        "env": {
          "ASSIMP_CMD_PATH": "/path/to/assimp"
        }
      }
    }
  }
}
```

#### Using Model Context Protocol CLI

```bash
# Install mcp CLI
npm install -g @modelcontextprotocol/cli

# Start the server
mcp dev assimp-mcp-server
```

## Available Tools

### Information & Version

| Tool | Description | Parameters |
|------|-------------|-----------|
| `get_version` | Get Assimp version information | None |
| `list_supported_extensions` | List all supported import extensions | None |
| `check_extension_support` | Check if an extension is supported | `extension` |
| `list_export_formats` | List all supported export formats | None |
| `get_export_format_info` | Get info about a specific export format | `format_id` |

### Model Analysis

| Tool | Description | Parameters |
|------|-------------|-----------|
| `get_model_info` | Get detailed info about a 3D model | `file_path`, `verbose?`, `raw?` |

### Import/Export

| Tool | Description | Parameters |
|------|-------------|-----------|
| `export_model` | Export a model to different format | `input_path`, `output_path`, `format_id?`, `postprocess_flags?`, `rotation_*?`, `verbose?`, `show_log?` |

### Texture Handling

| Tool | Description | Parameters |
|------|-------------|-----------|
| `extract_texture` | Extract embedded texture | `input_path`, `texture_index?`, `output_path?` |

### Dump & Comparison

| Tool | Description | Parameters |
|------|-------------|-----------|
| `create_dump` | Create model dump (XML/Binary) | `input_path`, `output_path?`, `format?`, `silent?`, `binary?` |
| `compare_dumps` | Compare two model dumps | `dump1_path`, `dump2_path`, `tolerance?` |

### Testing

| Tool | Description | Parameters |
|------|-------------|-----------|
| `batch_import_test` | Test batch loading of files | `file_paths` |

## Resources

The server provides the following MCP resources:

- `assimp://supported_formats` - List of all supported import/export formats
- `assimp://version_info` - Version information for the Assimp library

## Configuration

### Environment Variables

- `ASSIMP_CMD_PATH`: Path to the Assimp CLI executable
- `ASSIMP_TEMP_DIR`: Temporary directory for file operations (optional)

### Build Integration

To integrate with Assimp's CMake build system, add this to your CMakeLists.txt:

```cmake
add_subdirectory(tools/mcp_server)
```

## Post-Processing Flags

The `export_model` tool supports these post-processing flags:

- `pretransform-vertices`
- `gen-smooth-normals`
- `gen-normals`
- `calc-tangent-space`
- `join-identical-vertices`
- `remove-redundant-materials`
- `find-degenerates`
- `split-large-meshes`
- `limit-bone-weights`
- `validate-data-structure`
- `improve-cache-locality`
- `sort-by-ptype`
- `convert-to-lh`
- `flip-uv`
- `flip-winding-order`
- `transform-uv-coords`
- `gen-uvcoords`
- `find-invalid-data`
- `fix-normals`
- `triangulate`
- `find-instances`
- `optimize-graph`
- `optimize-meshes`
- `debone`
- `split-by-bone-count`
- `embed-textures`
- `global-scale`

## Examples

### Get Assimp Version

```python
# Using MCP client
result = await client.call_tool("get_version", {})
print(result)
```

### List Supported Formats

```python
formats = await client.call_tool("list_supported_extensions", {})
print("Supported formats:", formats)
```

### Get Model Information

```python
info = await client.call_tool("get_model_info", {
    "file_path": "/path/to/model.obj",
    "verbose": True
})
print(info)
```

### Export Model

```python
result = await client.call_tool("export_model", {
    "input_path": "/path/to/model.fbx",
    "output_path": "/path/to/model.obj",
    "postprocess_flags": ["triangulate", "gen-normals"],
    "rotation_y": 90.0
})
```

### Extract Texture

```python
result = await client.call_tool("extract_texture", {
    "input_path": "/path/to/model.gltf",
    "texture_index": 0,
    "output_path": "/path/to/texture.png"
})
```

## Development

### Project Structure

```
tools/mcp_server/
├── __init__.py          # Package initialization
├── server.py           # Main MCP server implementation
├── requirements.txt    # Python dependencies
├── pyproject.toml      # Build configuration
└── README.md           # This file
```

### Running Tests

```bash
# Install dev dependencies
pip install -e ".[dev]"

# Run tests
pytest
```

### Code Formatting

```bash
# Format code
black mcp_server/
ruff check mcp_server/
```

## License

This MCP server is licensed under the BSD 3-Clause License, same as the Assimp library.

```
Copyright (c) 2006-2026, assimp team

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the assimp team nor the names of its contributors may
   be used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
```

## Contributing

Contributions are welcome! Please see the main [Assimp repository](https://github.com/assimp/assimp) for contribution guidelines.

## Acknowledgments

- [Assimp Team](https://github.com/assimp/assimp) for the original library
- [Model Context Protocol](https://modelcontextprotocol.io/) for the protocol specification
- All contributors and users of the Assimp library

## Support

For issues and questions:

- **Assimp Issues**: [GitHub Issues](https://github.com/assimp/assimp/issues)
- **MCP Protocol**: [MCP Documentation](https://modelcontextprotocol.io/)
- **Discussions**: [Assimp Discussions](https://github.com/assimp/assimp/discussions)
