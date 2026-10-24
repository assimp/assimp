#!/usr/bin/env python3
"""
Assimp MCP Server

A Model Context Protocol server that wraps the Assimp command-line tool
to provide programmatic access to 3D model import/export functionality.

This server exposes Assimp's CLI commands as MCP tools, allowing
applications to:
- Import 3D models from various formats
- Export models to different formats
- Get information about supported formats
- Extract embedded textures
- Compare model dumps
"""

import asyncio
import json
import os
import subprocess
import tempfile
import shutil
from typing import Any, Dict, List, Optional, Union
from pathlib import Path

from mcp.server.fastmcp import FastMCP
from mcp.server.fastmcp.resources import Resource
from mcp.types import TextContent, ImageContent, EmbeddedResource


class AssimpMCPServer:
    """Assimp MCP Server implementation."""

    def __init__(self):
        self._assimp_path: Optional[str] = None
        self._temp_dir: Optional[str] = None

    def _find_assimp_executable(self) -> Optional[str]:
        """Find the assimp CLI executable."""
        # Check environment variable first
        env_path = os.environ.get("ASSIMP_CMD_PATH")
        if env_path and os.path.exists(env_path):
            return env_path
        
        # Try common names
        names = ["assimp", "assimp_cmd", "assimp.exe", "assimpd.exe"]
        for name in names:
            # Check in PATH
            path_result = shutil.which(name)
            if path_result:
                return path_result
            
            # Check in build directories (for development)
            build_paths = [
                "C:\\development\\projects\\assimp\\build\\tools\\assimp_cmd\\Debug\\assimp.exe",
                "C:\\development\\projects\\assimp\\build\\tools\\assimp_cmd\\Release\\assimp.exe",
                "build\\tools\\assimp_cmd\\Debug\\assimp.exe",
                "build\\tools\\assimp_cmd\\Release\\assimp.exe",
                "C:\\development\\projects\\assimp\\bin\\Debug\\assimpd.exe",
                "C:\\development\\projects\\assimp\\bin\\Release\\assimp.exe",
                "bin\\Debug\\assimpd.exe",
                "bin\\Release\\assimp.exe",
            ]
            for build_path in build_paths:
                if os.path.exists(build_path):
                    return build_path
        
        return None

    async def initialize_server(self) -> Dict[str, Any]:
        """Initialize the MCP server."""
        # Create temporary directory for file operations
        self._temp_dir = tempfile.mkdtemp(prefix="assimp_mcp_")
        
        # Try to find assimp executable
        self._assimp_path = self._find_assimp_executable()
        
        if not self._assimp_path:
            raise RuntimeError(
                "Assimp CLI tool not found. Please ensure assimp is installed "
                "and available in PATH, or set ASSIMP_CMD_PATH environment variable."
            )
        
        # Verify the assimp tool works
        try:
            result = await self._run_assimp_command(["version"])
            if result.returncode != 0:
                raise RuntimeError(f"Assimp CLI tool failed: {result.stderr}")
        except Exception as e:
            raise RuntimeError(f"Failed to verify Assimp CLI tool: {e}")
        
        return {
            "protocolVersion": "2024-11-05",
            "capabilities": {
                "tools": {},
                "resources": {},
                "prompts": {},
            },
            "serverInfo": {
                "name": "assimp-mcp-server",
                "version": "1.0.0",
            },
        }

    async def shutdown_server(self) -> None:
        """Clean up resources on shutdown."""
        if self._temp_dir and os.path.exists(self._temp_dir):
            shutil.rmtree(self._temp_dir, ignore_errors=True)
        self._temp_dir = None

    async def _run_assimp_command(
        self, 
        args: List[str], 
        input_file: Optional[str] = None,
        output_file: Optional[str] = None,
        cwd: Optional[str] = None
    ) -> subprocess.CompletedProcess:
        """Run an assimp CLI command."""
        cmd = [self._assimp_path] + args
        
        # Create a temporary directory for this operation if needed
        temp_dir = self._temp_dir or tempfile.mkdtemp()
        
        try:
            process = await asyncio.create_subprocess_exec(
                *cmd,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
                cwd=cwd or temp_dir,
                env=os.environ.copy(),
            )
            
            stdout, stderr = await process.communicate()
            
            return subprocess.CompletedProcess(
                args=cmd,
                returncode=process.returncode,
                stdout=stdout.decode('utf-8', errors='replace'),
                stderr=stderr.decode('utf-8', errors='replace'),
            )
        except Exception as e:
            # Return a failed process
            return subprocess.CompletedProcess(
                args=cmd,
                returncode=-1,
                stdout="",
                stderr=str(e),
            )

    async def _run_assimp_command_with_files(
        self,
        command: List[str],
        input_files: Optional[List[str]] = None,
        output_path: Optional[str] = None,
    ) -> Dict[str, Any]:
        """Run an assimp command with file handling."""
        args = command.copy()
        
        # Handle input files
        temp_files = []
        try:
            if input_files:
                for input_file in input_files:
                    # If it's a local file path, use it directly
                    if os.path.exists(input_file):
                        args.append(input_file)
                    else:
                        # For now, we assume input files are local paths
                        args.append(input_file)
            
            if output_path:
                args.extend(["-o", output_path])
            
            result = await self._run_assimp_command(args)
            
            return {
                "success": result.returncode == 0,
                "returncode": result.returncode,
                "stdout": result.stdout,
                "stderr": result.stderr,
                "command": " ".join(args),
            }
        finally:
            # Clean up temporary files
            for temp_file in temp_files:
                if os.path.exists(temp_file):
                    os.remove(temp_file)


# Create the server instance
server_instance = AssimpMCPServer()

# Create FastMCP app
app = FastMCP("assimp-mcp-server")


# ============================================================================
# MCP Tool Definitions
# ============================================================================

@app.tool()
async def get_version() -> str:
    """
    Get the Assimp version information.
    
    Returns:
        Version string containing Assimp version and build information.
    """
    result = await server_instance._run_assimp_command(["version"])
    if result.returncode == 0:
        return result.stdout.strip()
    else:
        raise RuntimeError(f"Failed to get version: {result.stderr}")


@app.tool()
async def list_supported_extensions() -> List[str]:
    """
    List all file extensions supported for import by Assimp.
    
    Returns:
        List of supported file extensions (e.g., ["obj", "fbx", "gltf", ...]).
    """
    result = await server_instance._run_assimp_command(["listext"])
    if result.returncode == 0:
        extensions = result.stdout.strip().split()
        return extensions
    else:
        raise RuntimeError(f"Failed to list extensions: {result.stderr}")


@app.tool()
async def check_extension_support(extension: str) -> Dict[str, bool]:
    """
    Check if a specific file extension is supported for import.
    
    Args:
        extension: The file extension to check (e.g., "obj", "fbx").
        
    Returns:
        Dictionary with 'supported' boolean indicating if the extension is recognized.
    """
    result = await server_instance._run_assimp_command(["knowext", extension])
    if result.returncode == 0:
        supported = "is known" in result.stdout.lower()
        return {"extension": extension, "supported": supported}
    else:
        raise RuntimeError(f"Failed to check extension: {result.stderr}")


@app.tool()
async def list_export_formats() -> List[str]:
    """
    List all supported export formats.
    
    Returns:
        List of export format identifiers (e.g., ["obj", "stl", "ply", ...]).
    """
    result = await server_instance._run_assimp_command(["listexport"])
    if result.returncode == 0:
        formats = result.stdout.strip().split('\n')
        return formats
    else:
        raise RuntimeError(f"Failed to list export formats: {result.stderr}")


@app.tool()
async def get_export_format_info(format_id: str) -> Dict[str, str]:
    """
    Get information about a specific export format.
    
    Args:
        format_id: The export format identifier (e.g., "obj", "stl").
        
    Returns:
        Dictionary containing format information (id, fileExtension, description).
    """
    result = await server_instance._run_assimp_command(["exportinfo", format_id])
    if result.returncode == 0:
        lines = result.stdout.strip().split('\n')
        if len(lines) >= 3:
            return {
                "id": lines[0],
                "fileExtension": lines[1],
                "description": lines[2],
            }
        else:
            return {"error": f"Unexpected output format: {result.stdout}"}
    else:
        raise RuntimeError(f"Failed to get export format info: {result.stderr}")


@app.tool()
async def get_model_info(
    file_path: str, 
    verbose: bool = False,
    raw: bool = False
) -> Dict[str, Any]:
    """
    Get information about a 3D model file.
    
    This command provides detailed statistics about a 3D model including:
    - Number of meshes, vertices, faces
    - Material information
    - Node hierarchy
    - Texture information
    - Animation data
    
    Args:
        file_path: Path to the 3D model file.
        verbose: If True, include detailed information like node transformations.
        raw: If True, perform raw import without post-processing.
        
    Returns:
        Dictionary containing model information.
    """
    args = ["info", file_path]
    if verbose:
        args.append("-v")
    if raw:
        args.append("-r")
    
    result = await server_instance._run_assimp_command(args)
    if result.returncode == 0:
        return {
            "file": file_path,
            "info": result.stdout,
            "success": True,
        }
    else:
        raise RuntimeError(f"Failed to get model info: {result.stderr}")


@app.tool()
async def export_model(
    input_path: str,
    output_path: str,
    format_id: Optional[str] = None,
    postprocess_flags: Optional[List[str]] = None,
    rotation_x: float = 0.0,
    rotation_y: float = 0.0,
    rotation_z: float = 0.0,
    verbose: bool = False,
    show_log: bool = False,
) -> Dict[str, Any]:
    """
    Export a 3D model to a different format.
    
    Args:
        input_path: Path to the input 3D model file.
        output_path: Path to save the exported file.
        format_id: Optional export format identifier. If not specified,
                          the format is determined from the output file extension.
        postprocess_flags: List of post-processing flags to apply.
                          Common flags: "triangulate", "gen-normals", 
                          "gen-smooth-normals", "flip-uv", "pretransform-vertices".
        rotation_x: Rotation angle in degrees around X axis.
        rotation_y: Rotation angle in degrees around Y axis.
        rotation_z: Rotation angle in degrees around Z axis.
        verbose: Enable verbose logging.
        show_log: Show detailed import/export logs.
        
    Returns:
        Dictionary containing export result information.
    """
    args = ["export", input_path, output_path]
    
    # Add format if specified
    if format_id:
        args.extend(["-f", format_id])
    
    # Add post-processing flags
    if postprocess_flags:
        flag_mapping = {
            "pretransform-vertices": "-ptv",
            "gen-smooth-normals": "-gsn",
            "gen-normals": "-gn",
            "calc-tangent-space": "-cts",
            "join-identical-vertices": "-jiv",
            "remove-redundant-materials": "-rrm",
            "find-degenerates": "-fd",
            "split-large-meshes": "-slm",
            "limit-bone-weights": "-lbw",
            "validate-data-structure": "-vds",
            "improve-cache-locality": "-icl",
            "sort-by-ptype": "-sbpt",
            "convert-to-lh": "-lh",
            "flip-uv": "-fuv",
            "flip-winding-order": "-fwo",
            "transform-uv-coords": "-tuv",
            "gen-uvcoords": "-guv",
            "find-invalid-data": "-fid",
            "fix-normals": "-fixn",
            "triangulate": "-tri",
            "find-instances": "-fi",
            "optimize-graph": "-og",
            "optimize-meshes": "-om",
            "debone": "-db",
            "split-by-bone-count": "-sbc",
            "embed-textures": "-embtex",
            "global-scale": "-gs",
        }
        for flag in postprocess_flags:
            if flag in flag_mapping:
                args.append(flag_mapping[flag])
    
    # Add rotation
    if rotation_x != 0.0:
        args.append(f"--rotation-x={rotation_x}")
    if rotation_y != 0.0:
        args.append(f"--rotation-y={rotation_y}")
    if rotation_z != 0.0:
        args.append(f"--rotation-z={rotation_z}")
    
    # Add logging options
    if verbose:
        args.append("-v")
    if show_log:
        args.append("-l")
    
    result = await server_instance._run_assimp_command(args)
    
    if result.returncode == 0:
        return {
            "input_file": input_path,
            "output_file": output_path,
            "format": format_id,
            "success": True,
            "output": result.stdout,
            "warnings": result.stderr,
        }
    else:
        return {
            "input_file": input_path,
            "output_file": output_path,
            "format": format_id,
            "success": False,
            "error": result.stderr,
            "output": result.stdout,
        }


@app.tool()
async def extract_texture(
    input_path: str,
    texture_index: int = 0,
    output_path: Optional[str] = None,
) -> Dict[str, Any]:
    """
    Extract an embedded texture from a 3D model file.
    
    Args:
        input_path: Path to the 3D model file.
        texture_index: Index of the texture to extract (default: 0).
        output_path: Path to save the extracted texture. If not specified,
                    the texture will be saved to a temporary file.
                    
    Returns:
        Dictionary containing extraction result information.
    """
    args = ["extract", input_path, str(texture_index)]
    
    if output_path:
        args.append(output_path)
    
    result = await server_instance._run_assimp_command(args)
    
    if result.returncode == 0:
        output_file = output_path or f"{input_path}_texture_{texture_index}.png"
        return {
            "input_file": input_path,
            "texture_index": texture_index,
            "output_file": output_file,
            "success": True,
            "output": result.stdout,
        }
    else:
        return {
            "input_file": input_path,
            "texture_index": texture_index,
            "success": False,
            "error": result.stderr,
        }


@app.tool()
async def create_dump(
    input_path: str,
    output_path: Optional[str] = None,
    format: str = "xml",
    silent: bool = False,
    binary: bool = False,
) -> Dict[str, Any]:
    """
    Create a dump file from a 3D model.
    
    Dumps can be in XML (ASSXML) or binary (ASSBIN) format and contain
    the complete scene structure.
    
    Args:
        input_path: Path to the 3D model file.
        output_path: Path to save the dump file. If not specified,
                    uses input filename with .assxml or .assbin extension.
        format: Dump format - "xml" for ASSXML or "bin" for ASSBIN.
        silent: If True, suppress progress output.
        binary: If True, create binary dump (ASSBIN). Overrides format.
        
    Returns:
        Dictionary containing dump creation result.
    """
    args = ["dump", input_path]
    
    # Set output path
    if output_path:
        args.append(output_path)
    
    # Set format flags
    if binary:
        args.append("-sb")  # Save binary
    elif format == "bin":
        args.append("-sb")
    # XML is default, no flag needed
    
    if silent:
        args.append("-s")
    
    result = await server_instance._run_assimp_command(args)
    
    if result.returncode == 0:
        output_file = output_path or input_path + (".assbin" if binary else ".assxml")
        return {
            "input_file": input_path,
            "output_file": output_file,
            "format": "binary" if binary else "xml",
            "success": True,
            "output": result.stdout,
        }
    else:
        return {
            "input_file": input_path,
            "success": False,
            "error": result.stderr,
        }


@app.tool()
async def compare_dumps(
    dump1_path: str,
    dump2_path: str,
    tolerance: float = 1e-5,
) -> Dict[str, Any]:
    """
    Compare two model dump files.
    
    This is useful for regression testing to ensure that model imports
    produce consistent results.
    
    Args:
        dump1_path: Path to the first dump file.
        dump2_path: Path to the second dump file.
        tolerance: Numerical tolerance for comparing floating-point values.
        
    Returns:
        Dictionary containing comparison result.
    """
    # For cmpdump, we need to use the format: assimp cmpdump <dump1> <dump2> <tolerance>
    args = ["cmpdump", dump1_path, dump2_path, str(tolerance)]
    
    result = await server_instance._run_assimp_command(args)
    
    if result.returncode == 0:
        return {
            "dump1": dump1_path,
            "dump2": dump2_path,
            "tolerance": tolerance,
            "success": True,
            "identical": "comparison succeeded" in result.stdout.lower(),
            "output": result.stdout,
        }
    else:
        return {
            "dump1": dump1_path,
            "dump2": dump2_path,
            "tolerance": tolerance,
            "success": False,
            "identical": False,
            "error": result.stderr,
        }


@app.tool()
async def batch_import_test(
    file_paths: List[str],
) -> Dict[str, Any]:
    """
    Test batch loading of multiple model files.
    
    This is useful for checking compatibility of multiple files
    with the same importer instance.
    
    Args:
        file_paths: List of paths to 3D model files to test.
        
    Returns:
        Dictionary containing batch test results.
    """
    args = ["testbatchload"] + file_paths
    
    result = await server_instance._run_assimp_command(args)
    
    if result.returncode == 0:
        return {
            "file_count": len(file_paths),
            "success": True,
            "output": result.stdout,
        }
    else:
        return {
            "file_count": len(file_paths),
            "success": False,
            "error": result.stderr,
        }


# ============================================================================
# Resource Definitions
# ============================================================================

class SupportedFormatsResource(Resource):
    """Resource for listing supported formats."""
    
    uri: str = "assimp://supported_formats"
    name: str = "assimp://supported_formats"
    description: str = "List of all supported import/export formats"
    mime_type: str = "text/markdown"
    
    async def read(self) -> str:
        """Read the resource content."""
        extensions = await list_supported_extensions()
        export_formats = await list_export_formats()
        
        content = f"""# Assimp Supported Formats

## Import Formats
The following file extensions are supported for import:

{', '.join(extensions)}

## Export Formats
The following formats are supported for export:

{', '.join(export_formats)}

For more information about a specific export format, use the get_export_format_info tool.
"""
        return content


class VersionInfoResource(Resource):
    """Resource for version information."""
    
    uri: str = "assimp://version_info"
    name: str = "assimp://version_info"
    description: str = "Version information for the Assimp library"
    mime_type: str = "text/plain"
    
    async def read(self) -> str:
        """Read the resource content."""
        version = await get_version()
        return version


# Add resources to the server
app.add_resource(SupportedFormatsResource())
app.add_resource(VersionInfoResource())


# ============================================================================
# Main Entry Point
# ============================================================================

async def main():
    """Main entry point for the MCP server."""
    # Initialize the server
    await server_instance.initialize_server()
    
    try:
        # Run the server - use the async version
        await app.run_stdio_async()
    finally:
        # Clean up on exit
        await server_instance.shutdown_server()


if __name__ == "__main__":
    import sys
    if sys.platform == "win32":
        # On Windows, we need to use asyncio.run for the main function
        asyncio.run(main())
    else:
        # On Unix, we can use the standard approach
        asyncio.run(main())