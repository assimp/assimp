# Open Asset Import Library (assimp)
# MCP Server CMake Configuration
# ----------------------------------------------------------------------------
#
# Copyright (c) 2006-2026, assimp team
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of the assimp team, nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission of the assimp team.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#
# ----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.10)

project(assimp-mcp-server LANGUAGES NONE)

# Find Python
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

# Find pip
find_program(PIP_EXECUTABLE pip3 HINTS ${Python3_EXECUTABLE} PATH_SUFFIXES Scripts)

# Set up the MCP server directory
set(MCP_SERVER_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MCP_SERVER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/mcp_server)

# Create output directory
file(MAKE_DIRECTORY ${MCP_SERVER_OUTPUT_DIR})

# Configure the server.py file with correct paths
configure_file(
    ${MCP_SERVER_DIR}/server.py
    ${MCP_SERVER_OUTPUT_DIR}/server.py
    COPYONLY
)

# Copy other files
file(COPY ${MCP_SERVER_DIR}/__init__.py DESTINATION ${MCP_SERVER_OUTPUT_DIR})
file(COPY ${MCP_SERVER_DIR}/README.md DESTINATION ${MCP_SERVER_OUTPUT_DIR})
file(COPY ${MCP_SERVER_DIR}/requirements.txt DESTINATION ${MCP_SERVER_OUTPUT_DIR})
file(COPY ${MCP_SERVER_DIR}/pyproject.toml DESTINATION ${MCP_SERVER_OUTPUT_DIR})

# Install dependencies
add_custom_target(mcp_server_install_dependencies
    COMMAND ${PIP_EXECUTABLE} install -r ${MCP_SERVER_OUTPUT_DIR}/requirements.txt
    WORKING_DIRECTORY ${MCP_SERVER_OUTPUT_DIR}
    COMMENT "Installing MCP server dependencies"
    BYPRODUCT ${MCP_SERVER_OUTPUT_DIR}
)

# Create a script to run the server
set(SERVER_SCRIPT ${MCP_SERVER_OUTPUT_DIR}/run_mcp_server.py)
file(WRITE ${SERVER_SCRIPT} """
#!/usr/bin/env python3
import os
import sys
import subprocess
import platform

def find_assimp_executable():
    """Find the assimp CLI executable."""
    # Check if built in this build
    build_dirs = [
        "${CMAKE_BINARY_DIR}/tools/assimp_cmd/Debug",
        "${CMAKE_BINARY_DIR}/tools/assimp_cmd/Release",
        "${CMAKE_BINARY_DIR}/Debug",
        "${CMAKE_BINARY_DIR}/Release",
    ]
    
    if platform.system() == "Windows":
        executable_names = ["assimp.exe"]
    else:
        executable_names = ["assimp", "assimp_cmd"]
    
    for build_dir in build_dirs:
        for exec_name in executable_names:
            path = os.path.join(build_dir, exec_name)
            if os.path.exists(path):
                return path
    
    # Try to find in PATH
    for exec_name in executable_names:
        try:
            result = subprocess.run([exec_name, "version"], 
                                  capture_output=True, text=True, timeout=5)
            if result.returncode == 0:
                return exec_name
        except:
            pass
    
    return None

if __name__ == "__main__":
    # Set up environment
    assimp_path = find_assimp_executable()
    if assimp_path:
        os.environ["ASSIMP_CMD_PATH"] = assimp_path
        print(f"Using Assimp CLI: {assimp_path}")
    
    # Run the MCP server
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from server import main
    import asyncio
    asyncio.run(main())
""")

# Make the script executable on Unix-like systems
if(UNIX AND NOT APPLE)
    execute_process(COMMAND chmod +x ${SERVER_SCRIPT})
endif()

# Create a target to run the MCP server
add_custom_target(mcp_server_run
    COMMAND ${Python3_EXECUTABLE} ${SERVER_SCRIPT}
    WORKING_DIRECTORY ${MCP_SERVER_OUTPUT_DIR}
    COMMENT "Running MCP server"
    DEPENDS mcp_server_install_dependencies
)

# Create a wrapper executable (on Windows)
if(WIN32)
    # Create a batch file for easy execution
    set(BATCH_FILE ${MCP_SERVER_OUTPUT_DIR}/assimp-mcp-server.bat)
    file(WRITE ${BATCH_FILE} """
@echo off
python "${SERVER_SCRIPT}"
""")
    
    # Create a Python entry point script
    set(ENTRY_POINT ${MCP_SERVER_OUTPUT_DIR}/assimp_mcp_server.py)
    file(WRITE ${ENTRY_POINT} """
#!/usr/bin/env python3
import os
import sys

# Add the current directory to path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

# Import and run
from server import main
import asyncio

if __name__ == "__main__":
    asyncio.run(main())
""")
endif()

# Install the MCP server as a Python package (optional)
add_custom_target(mcp_server_install
    COMMAND ${PIP_EXECUTABLE} install -e ${MCP_SERVER_OUTPUT_DIR}
    WORKING_DIRECTORY ${MCP_SERVER_OUTPUT_DIR}
    COMMENT "Installing MCP server as editable package"
    DEPENDS mcp_server_install_dependencies
)

# Output information
message(STATUS "MCP Server configuration:")
message(STATUS "  Output directory: ${MCP_SERVER_OUTPUT_DIR}")
message(STATUS "  Python executable: ${Python3_EXECUTABLE}")
message(STATUS "  Server script: ${SERVER_SCRIPT}")

if(WIN32)
    message(STATUS "  Batch file: ${BATCH_FILE}")
    message(STATUS "  Entry point: ${ENTRY_POINT}")
endif()

# Add custom command for building and running
add_custom_target(mcp_server
    DEPENDS mcp_server_install_dependencies
    COMMENT "MCP Server setup complete. Run with: python ${SERVER_SCRIPT}"
)
