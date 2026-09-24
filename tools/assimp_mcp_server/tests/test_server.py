#!/usr/bin/env python3
"""
Tests for Assimp MCP Server

These tests verify the functionality of the MCP server by testing
the individual tool methods.
"""

import asyncio
import os
import tempfile
import shutil
from unittest.mock import AsyncMock, patch, MagicMock
import pytest

# Add parent directory to path for imports
import sys
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from mcp_server.server import AssimpMCPServer


@pytest.fixture
def temp_dir():
    """Create a temporary directory for tests."""
    temp_path = tempfile.mkdtemp(prefix="assimp_mcp_test_")
    yield temp_path
    shutil.rmtree(temp_path, ignore_errors=True)


@pytest.fixture
def mock_server(temp_dir):
    """Create a mock MCP server for testing."""
    server = AssimpMCPServer("test-server")
    server._temp_dir = temp_dir
    server._assimp_path = "assimp"  # Mock path
    return server


class TestServerInitialization:
    """Test server initialization and setup."""

    @pytest.mark.asyncio
    async def test_find_assimp_executable_env(self, temp_dir):
        """Test finding assimp executable from environment variable."""
        with patch.dict(os.environ, {"ASSIMP_CMD_PATH": "/custom/path/assimp"}):
            server = AssimpMCPServer("test")
            path = server._find_assimp_executable()
            assert path == "/custom/path/assimp"

    @pytest.mark.asyncio
    async def test_find_assimp_executable_in_path(self, temp_dir):
        """Test finding assimp executable in PATH."""
        with patch("shutil.which", return_value="/usr/bin/assimp"):
            server = AssimpMCPServer("test")
            path = server._find_assimp_executable()
            assert path == "/usr/bin/assimp"

    @pytest.mark.asyncio
    async def test_find_assimp_executable_not_found(self, temp_dir):
        """Test when assimp executable is not found."""
        with patch("shutil.which", return_value=None):
            with patch.dict(os.environ, {}, clear=True):
                server = AssimpMCPServer("test")
                path = server._find_assimp_executable()
                assert path is None


class TestToolMethods:
    """Test the MCP tool methods."""

    @pytest.mark.asyncio
    async def test_get_version(self, mock_server):
        """Test get_version tool."""
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "Assimp version 5.4.0"
        mock_result.stderr = ""
        
        with patch.object(mock_server, '_run_assimp_command', new_callable=AsyncMock) as mock_run:
            mock_run.return_value = mock_result
            version = await mock_server.get_version()
            assert version == "Assimp version 5.4.0"
            mock_run.assert_called_once_with(["version"])

    @pytest.mark.asyncio
    async def test_get_version_failure(self, mock_server):
        """Test get_version tool failure."""
        mock_result = MagicMock()
        mock_result.returncode = 1
        mock_result.stdout = ""
        mock_result.stderr = "Error getting version"
        
        with patch.object(mock_server, '_run_assimp_command', new_callable=AsyncMock) as mock_run:
            mock_run.return_value = mock_result
            with pytest.raises(RuntimeError):
                await mock_server.get_version()

    @pytest.mark.asyncio
    async def test_list_supported_extensions(self, mock_server):
        """Test list_supported_extensions tool."""
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "obj fbx gltf stl ply"
        mock_result.stderr = ""
        
        with patch.object(mock_server, '_run_assimp_command', new_callable=AsyncMock) as mock_run:
            mock_run.return_value = mock_result
            extensions = await mock_server.list_supported_extensions()
            assert extensions == ["obj", "fbx", "gltf", "stl", "ply"]
            mock_run.assert_called_once_with(["listext"])

    @pytest.mark.asyncio
    async def test_check_extension_support(self, mock_server):
        """Test check_extension_support tool."""
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "File extension 'obj' is known"
        mock_result.stderr = ""
        
        with patch.object(mock_server, '_run_assimp_command', new_callable=AsyncMock) as mock_run:
            mock_run.return_value = mock_result
            result = await mock_server.check_extension_support("obj")
            assert result["extension"] == "obj"
            assert result["supported"] is True
            mock_run.assert_called_once_with(["knowext", "obj"])

    @pytest.mark.asyncio
    async def test_check_extension_support_unsupported(self, mock_server):
        """Test check_extension_support tool with unsupported extension."""
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "File extension 'xyz' is not known"
        mock_result.stderr = ""
        
        with patch.object(mock_server, '_run_assimp_command', new_callable=AsyncMock) as mock_run:
            mock_run.return_value = mock_result
            result = await mock_server.check_extension_support("xyz")
            assert result["extension"] == "xyz"
            assert result["supported"] is False

    @pytest.mark.asyncio
    async def test_list_export_formats(self, mock_server):
        """Test list_export_formats tool."""
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "obj\nstl\nply\n"
        mock_result.stderr = ""
        
        with patch.object(mock_server, '_run_assimp_command', new_callable=AsyncMock) as mock_run:
            mock_run.return_value = mock_result
            formats = await mock_server.list_export_formats()
            assert formats == ["obj", "stl", "ply"]
            mock_run.assert_called_once_with(["listexport"])

    @pytest.mark.asyncio
    async def test_get_export_format_info(self, mock_server):
        """Test get_export_format_info tool."""
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "obj\n.obj\nWavefront OBJ format"
        mock_result.stderr = ""
        
        with patch.object(mock_server, '_run_assimp_command', new_callable=AsyncMock) as mock_run:
            mock_run.return_value = mock_result
            info = await mock_server.get_export_format_info("obj")
            assert info["id"] == "obj"
            assert info["fileExtension"] == ".obj"
            assert info["description"] == "Wavefront OBJ format"
            mock_run.assert_called_once_with(["exportinfo", "obj"])

    @pytest.mark.asyncio
    async def test_get_model_info(self, mock_server, temp_dir):
        """Test get_model_info tool."""
        # Create a temporary model file
        model_path = os.path.join(temp_dir, "test.obj")
        with open(model_path, "w") as f:
            f.write("# Test OBJ file\nv 0 0 0\n")
        
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "Model info: 1 vertices, 0 faces"
        mock_result.stderr = ""
        
        with patch.object(mock_server, '_run_assimp_command', new_callable=AsyncMock) as mock_run:
            mock_run.return_value = mock_result
            info = await mock_server.get_model_info(model_path)
            assert info["file"] == model_path
            assert info["success"] is True
            mock_run.assert_called_once_with(["info", model_path])

    @pytest.mark.asyncio
    async def test_get_model_info_verbose(self, mock_server, temp_dir):
        """Test get_model_info tool with verbose flag."""
        model_path = os.path.join(temp_dir, "test.obj")
        with open(model_path, "w") as f:
            f.write("# Test OBJ file\nv 0 0 0\n")
        
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "Detailed model info"
        mock_result.stderr = ""
        
        with patch.object(mock_server, '_run_assimp_command', new_callable=AsyncMock) as mock_run:
            mock_run.return_value = mock_result
            info = await mock_server.get_model_info(model_path, verbose=True, raw=True)
            assert info["file"] == model_path
            mock_run.assert_called_once_with(["info", model_path, "-v", "-r"])

    @pytest.mark.asyncio
    async def test_export_model(self, mock_server, temp_dir):
        """Test export_model tool."""
        input_path = os.path.join(temp_dir, "input.fbx")
        output_path = os.path.join(temp_dir, "output.obj")
        
        with open(input_path, "w") as f:
            f.write("# Test FBX file\n")
        
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "Export successful"
        mock_result.stderr = ""
        
        with patch.object(mock_server, '_run_assimp_command', new_callable=AsyncMock) as mock_run:
            mock_run.return_value = mock_result
            result = await mock_server.export_model(
                input_path=input_path,
                output_path=output_path,
                postprocess_flags=["triangulate"],
                rotation_y=90.0
            )
            assert result["success"] is True
            assert result["input_file"] == input_path
            assert result["output_file"] == output_path
            mock_run.assert_called_once()
            args = mock_run.call_args[0][0]
            assert "export" in args
            assert input_path in args
            assert output_path in args
            assert "-tri" in args
            assert "--rotation-y=90.0" in args

    @pytest.mark.asyncio
    async def test_export_model_with_format(self, mock_server, temp_dir):
        """Test export_model tool with explicit format."""
        input_path = os.path.join(temp_dir, "input.fbx")
        output_path = os.path.join(temp_dir, "output")
        
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "Export successful"
        mock_result.stderr = ""
        
        with patch.object(mock_server, '_run_assimp_command', new_callable=AsyncMock) as mock_run:
            mock_run.return_value = mock_result
            result = await mock_server.export_model(
                input_path=input_path,
                output_path=output_path,
                format_id="stl"
            )
            assert result["success"] is True
            mock_run.assert_called_once()
            args = mock_run.call_args[0][0]
            assert "-f" in args
            assert "stl" in args

    @pytest.mark.asyncio
    async def test_extract_texture(self, mock_server, temp_dir):
        """Test extract_texture tool."""
        input_path = os.path.join(temp_dir, "model.gltf")
        output_path = os.path.join(temp_dir, "texture.png")
        
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "Texture extracted"
        mock_result.stderr = ""
        
        with patch.object(mock_server, '_run_assimp_command', new_callable=AsyncMock) as mock_run:
            mock_run.return_value = mock_result
            result = await mock_server.extract_texture(
                input_path=input_path,
                texture_index=0,
                output_path=output_path
            )
            assert result["success"] is True
            assert result["texture_index"] == 0
            mock_run.assert_called_once_with(["extract", input_path, "0", output_path])

    @pytest.mark.asyncio
    async def test_create_dump(self, mock_server, temp_dir):
        """Test create_dump tool."""
        input_path = os.path.join(temp_dir, "model.obj")
        output_path = os.path.join(temp_dir, "model.assxml")
        
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "Dump created"
        mock_result.stderr = ""
        
        with patch.object(mock_server, '_run_assimp_command', new_callable=AsyncMock) as mock_run:
            mock_run.return_value = mock_result
            result = await mock_server.create_dump(
                input_path=input_path,
                output_path=output_path,
                format="xml",
                silent=True
            )
            assert result["success"] is True
            assert result["format"] == "xml"
            mock_run.assert_called_once_with(["dump", input_path, output_path, "-s"])

    @pytest.mark.asyncio
    async def test_create_dump_binary(self, mock_server, temp_dir):
        """Test create_dump tool with binary format."""
        input_path = os.path.join(temp_dir, "model.obj")
        
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "Binary dump created"
        mock_result.stderr = ""
        
        with patch.object(mock_server, '_run_assimp_command', new_callable=AsyncMock) as mock_run:
            mock_run.return_value = mock_result
            result = await mock_server.create_dump(
                input_path=input_path,
                binary=True
            )
            assert result["success"] is True
            assert result["format"] == "binary"
            mock_run.assert_called_once_with(["dump", input_path, "-sb"])

    @pytest.mark.asyncio
    async def test_compare_dumps(self, mock_server, temp_dir):
        """Test compare_dumps tool."""
        dump1_path = os.path.join(temp_dir, "dump1.assxml")
        dump2_path = os.path.join(temp_dir, "dump2.assxml")
        
        for path in [dump1_path, dump2_path]:
            with open(path, "w") as f:
                f.write("<assimp><scene></scene></assimp>")
        
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "comparison succeeded"
        mock_result.stderr = ""
        
        with patch.object(mock_server, '_run_assimp_command', new_callable=AsyncMock) as mock_run:
            mock_run.return_value = mock_result
            result = await mock_server.compare_dumps(
                dump1_path=dump1_path,
                dump2_path=dump2_path,
                tolerance=1e-5
            )
            assert result["success"] is True
            assert result["identical"] is True
            mock_run.assert_called_once_with(["cmpdump", dump1_path, dump2_path, "1e-05"])

    @pytest.mark.asyncio
    async def test_batch_import_test(self, mock_server, temp_dir):
        """Test batch_import_test tool."""
        file_paths = [
            os.path.join(temp_dir, "model1.obj"),
            os.path.join(temp_dir, "model2.fbx"),
        ]
        
        for path in file_paths:
            with open(path, "w") as f:
                f.write("# Test file\n")
        
        mock_result = MagicMock()
        mock_result.returncode = 0
        mock_result.stdout = "Batch test completed"
        mock_result.stderr = ""
        
        with patch.object(mock_server, '_run_assimp_command', new_callable=AsyncMock) as mock_run:
            mock_run.return_value = mock_result
            result = await mock_server.batch_import_test(file_paths)
            assert result["success"] is True
            assert result["file_count"] == 2
            mock_run.assert_called_once_with(["testbatchload"] + file_paths)


class TestResources:
    """Test MCP resource methods."""

    @pytest.mark.asyncio
    async def test_list_resources(self, mock_server):
        """Test list_resources method."""
        resources = await mock_server.list_resources()
        assert "assimp://supported_formats" in resources
        assert "assimp://version_info" in resources

    @pytest.mark.asyncio
    async def test_read_resource_supported_formats(self, mock_server):
        """Test reading supported_formats resource."""
        # Mock the tool methods
        with patch.object(mock_server, 'list_supported_extensions', new_callable=AsyncMock) as mock_ext:
            with patch.object(mock_server, 'list_export_formats', new_callable=AsyncMock) as mock_exp:
                mock_ext.return_value = ["obj", "fbx"]
                mock_exp.return_value = ["obj", "stl"]
                
                resource = await mock_server.read_resource("assimp://supported_formats")
                assert resource.type == "text/markdown"
                assert "obj" in resource.text
                assert "fbx" in resource.text

    @pytest.mark.asyncio
    async def test_read_resource_version_info(self, mock_server):
        """Test reading version_info resource."""
        with patch.object(mock_server, 'get_version', new_callable=AsyncMock) as mock_version:
            mock_version.return_value = "Assimp 5.4.0"
            
            resource = await mock_server.read_resource("assimp://version_info")
            assert resource.type == "text/plain"
            assert resource.text == "Assimp 5.4.0"

    @pytest.mark.asyncio
    async def test_read_resource_unknown(self, mock_server):
        """Test reading unknown resource."""
        with pytest.raises(ValueError):
            await mock_server.read_resource("assimp://unknown")


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
