# SPDX-License-Identifier: Apache 2.0
#
# Requiement: Python 3.7+

# TODO
# - [ ] Refactor components

import os
import warnings
from pathlib import Path

from typing import Optional, Union, List, Any, IO
from enum import Enum, auto

#
# Local modules
#
from .compat_typing_extensions import Literal, TypeAlias
from . import version
from .prims import Prim

try:
    from . import ctinyusd
except ImportError:
    import warnings
    warnings.warn(
        "Failed to import native module `ctinyusd`(No corresponding dll/so exists?). Loading USDA/USDC/USDZ feature is disabled.")

FILE_LIKE: TypeAlias = Union[str, os.PathLike, IO[str], IO[bytes]]

try:
    from typeguard import typechecked
    is_typegurad_available = True
except ImportError:
    is_typegurad_available = False

    # no-op
    def typechecked(cls):
        return cls


try:
    import numpy as np
except:
    pass

try:
    import pandas as pd
except:
    pass


def is_ctinyusd_available():
    import importlib.util

    # Seems '.' prefix required for relative module
    if importlib.util.find_spec(".ctinyusd", package='tinyusdz'):
        return True

    return False


def is_typeguard_available():
    import importlib.util

    if importlib.util.find_spec("typeguard"):
        return True

    return False


def is_numpy_available():
    import importlib.util

    if importlib.util.find_spec("numpy"):
        return True

    return False


def is_pandas_available():
    import importlib.util

    if importlib.util.find_spec("pandas"):
        return True

    return False


__version__ = version

"""
USD types
"""


class Type(Enum):
    bool = auto()
    int8 = auto()
    uint8 = auto()
    int16 = auto()
    uint16 = auto()
    int32 = auto()
    uint32 = auto()
    int64 = auto()
    uint64 = auto()
    int2 = auto()
    uint2 = auto()
    int3 = auto()
    uint3 = auto()
    int4 = auto()
    uint4 = auto()
    string = auto()
    vector3h = auto()
    vector3f = auto()
    vector3d = auto()
    color3h = auto()
    color3f = auto()
    color3d = auto()
    matrix2d = auto()
    matrix3d = auto()
    matrix4d = auto()


class XformOpType(Enum):
    # matrix4d
    Transform = auto()

    # vector3
    Translate = auto()
    Scale = auto()

    # scalar
    RotateX = auto()
    RotateY = auto()
    RotateZ = auto()

    # vector3
    RotateXYZ = auto()
    RotateXZY = auto()
    RotateYXZ = auto()
    RotateYZX = auto()
    RotateZXY = auto()
    RotateZYX = auto()

    # quat
    Orient = auto()

    # special token
    ResetXformStack = auto()

# USD ValueBlock(`None` in USDA)


class ValueBlock:
    def __init__(self):
        pass


"""
USD types in literal
"""

USDType: TypeAlias = Literal[
    "token",
    "string",
    "asset"
    "dictionary",
    "timecode",
    "rel",  # Relationship
    "bool",
    "uchar",
    "int",
    "int64",
    "uint",
    "uint64",
    "float",
    "float2",
    "float3",
    "float4",
    "double",
    "double2",
    "double3",
    "double4",
    "half",
    "half2",
    "half3",
    "half4",
    "quath",
    "quatf",
    "quatd",
    "normal3h",
    "normal3f",
    "normal3d",
    "vector3h",
    "vector3f",
    "vector3d",
    "vector4h",
    "vector4f",
    "vector4d",
    "color3h",
    "color3f",
    "color3d",
    "color4h",
    "color4f",
    "color4d",
    "point3h",
    "point3f",
    "point3d",
    "point3h",
    "point3f",
    "point3d",
    "texCoord2h",
    "texCoord2f",
    "texCoord2d",
    "texCoord3h",
    "texCoord3f",
    "texCoord3d",
    "texCoord4h",
    "texCoord4f",
    "texCoord4d",
    "matrix2f",
    "matrix3f",
    "matrix4f",
    "matrix2d",
    "matrix3d",
    "matrix4d",
    "frame4d",
]
SpecifierType: TypeAlias = Literal["def", "over", "class"]
AxisType: TypeAlias = Literal["X", "Y", "Z"]

# Builtin Prim type
PrimType: TypeAlias = Literal[
    "Model",  # Generic Prim
    "Scope",
    # Geom
    "Xform",
    "Mesh",
    "BasisCurves",
    "Sphere",
    "Cube",
    "Cylinder",
    "Cone",
    "Capsule",
    "Points"
    "GeomSubset",
    "PointInstancer",
    "Camera",
    # Lux
    "SphereLight",
    "DomeLight",
    "CylinderLight",
    "DiskLight",
    "RectLight",
    "DistantLight",
    "GeometryLight",
    "PortalLight",
    "PluginLight",
    # Shader
    "Shader",
    "Material",
    # Skel
    "SkelRoot",
    "Skeleton",
    "SkelAnimation",
    "BlendShape"
]

# UsdPreviewSurface types.
# https://openusd.org/release/spec_usdpreviewsurface.html
#
# NOTE: defined in usdImaging in pxrUSD
#
ShaderNodeType: TypeAlias = Literal[
    "ShaderNode",  # Generic shader node
    "UsdPreviewSurface",
    "UsdUVTexture",
    "UsdPrimvarReader_float",
    "UsdPrimvarReader_float2",
    "UsdPrimvarReader_float3",
    "UsdPrimvarReader_float4",
    "UsdPrimvarReader_int",
    "UsdPrimvarReader_normal",
    "UsdPrimvarReader_vector",
    "UsdPrimvarReader_point",
    "UsdPrimvarReader_matrix"
]


"""
numpy-like ndarray for Attribute data(e.g. points, normals, ...)
"""


@typechecked
class NDArray:
    def __init__(self, dtype: USDType = "uint8"):

        self.dtype: USDType = "uint8"
        self.dim: int = 1  # In USD, 1D or 2D only for array data

        self._data = None

    def from_numpy(self, nddata):
        if not is_numpy_available():
            raise ImportError("numpy is not installed.")

        assert isinstance(nddata, np.ndarray)

        assert nddata.dim < 2, "USD supports up to 2D array data"

        self.dtype = nddata.dtype
        self.dim
        self._data = nddata

    def to_numpy(self):
        """Convert data to numpy ndarray

        Returns:
            numpy.ndarray: numpy ndarray object
        """

        if not is_numpy_available():
            raise ImportError("numpy is not installed.")

        if isinstance(self._data, np.ndarray):
            return self._data

        raise RuntimeError("TODO")


@typechecked
class Token:
    def __init__(self, tok: str):
        self.tok = tok


class Property:
    """Represents Prim property.
    Base class for Attribute and Relationship.
    """

    def __init__(self):
        pass

    def is_property(self):
        return isinstance(self, Attribute)

    def is_relationship(self):
        return isinstance(self, Relationship)


class Attribute(Property):
    def __init__(self):
        super().__init__()


class Relationship(Property):
    def __init__(self):
        super().__init__()

@typechecked
class XformOp:
    def __init__(self,
                 op_type: XformOpType = XformOpType.Translate,
                 value: Any = None):
        pass

        self.suffix = ""
        self.inverted = False

        self.op_type = op_type

        self._value = value


@typechecked
class Stage:

    def __init__(self):
        self._stage = None
        self.filename = ""

        self.upAxis: Optional[AxisType] = None
        self.metersPerUnit: Union[float, None] = None
        self.framesPerSecond: Union[float, None] = None
        self.defaultPrim: Union[str, None] = None

        self.primChildren: List[Prim] = []

    def export_to_string(self):
        """Export Stage as USDA Ascii string.

        Returns:
          str: USDA string
        """
        return "TODO: export_to_string"

    def __repr__(self):
        s = "Stage:\n"
        if self.upAxis is not None:
            s += "  upAxis {}\n".format(self.upAxis)

        if self.metersPerUnit is not None:
            s += "  metersPerUnit {}\n".format(self.metersPerUnit)

        if self.framesPerSecond is not None:
            s += "  framesPerSecond {}\n".format(self.framesPerSecond)

        if self.defaultPrim is not None:
            s += "  defaultPrim {}\n".format(self.defaultPrim)

        return s

    def __str__(self):
        return self.__repr__()


def is_usd(file_like: FILE_LIKE) -> bool:
    """Test if input filename is a USD(USDC/USDA/UDSZ) file

    Args:
        file_like (FILE_LIKE): File-like object(Filename, file path, binary data)

    Returns:
        bool: True if USD file
    """

    from . import usd_loader

    ok: bool = usd_loader.is_usd(file_like)

    return ok


def is_usda(filename: Union[Path, str]) -> bool:
    """Test if input filename is a USDA file

    Args:
        filename (Path or str): Filename

    Returns:
        bool: True if USDA file
    """

    raise RuntimeError("TODO")


def is_usdc(filename: Union[Path, str]) -> bool:
    """Test if input filename is a USDC file

    Args:
        filename (Path or str): Filename

    Returns:
        bool: True if USDC file
    """

    raise RuntimeError("TODO")

# def load_usd(filename: Union[Path, str]) -> Stage:
#    """Loads USDC/USDA/UDSZ from a file
#
#    Args:
#        filename (Path or str): Filename
#
#    Returns:
#        Stage: Stage object
#    """
#
#    if isinstance(filename, str):
#        filename = Path(filename)
#
#    if not filename.exists():
#        raise FileNotFoundError("File {} not found.".format(filename))
#
#    if not filename.is_file():
#        raise FileNotFoundError("File {} is not a file".format(filename))
#
#    # TODO: Implement
#    stage = Stage()
#    stage.filename = filename
#
#    return stage


def load_usd_from_string(input_str: str) -> Stage:
    """Loads USDA from string

    Args:
        input_str: Input string

    Returns:
        Stage: Stage object
    """

    # TODO: Implement
    stage = Stage()

    return stage


def load_usd_from_binary(input_binary: bytes) -> Stage:
    """Loads USDC/USDZ from binary

    Args:
        input_binary(bytes): Input binary data of USDC or USDZ

    Returns:
        Stage: Stage object
    """

    # TODO: Implement
    stage = Stage()

    return stage


@typechecked
def dumps(usd: Union[Stage, Prim],
          format: str = "usda",
          indent: int = 2) -> str:
    """Dump USD Stage or Prim tree to str.

    Args:
        format(str): dump format. `usda` only at this time.

    Returns:
        str: Dumped string.
    """

    return "TODO"

# TODO: Python 3.10+
# from typing_extensions import TypeAlias
# FILE_LIKE: TypeAlias =  Union[str, os.PathLike, TextIO]


@typechecked
def save(stage: Stage,
         file_like: Union[str, os.PathLike, IO[str], IO[bytes]],
         *,
         format: Literal["auto", "usda", "usdc", "usdz"] = "usda",
         indent: int = 2) -> None:
    """Save Stage to USD(ASCII only for now)

    Args:
        stage(Stage): Stage
        file_like(Union[str, os.PathLike, TextIO]): File like object
        format(str): USD format. Currently `usda`(ASCII) only
        indent(int): Indent size for ASCII output.
                     (applicable only for `usda` format)

    Returns:
        None. Exception will be raised when error.

    """

    from . import usd_saver

    usd_saver.save(stage, file_like, format=format, indent=indent)


@typechecked
def load(file_like: Union[str, os.PathLike, IO[str], IO[bytes]],
         *,
         format: Literal["auto", "usda", "usdc", "usdz"] = "auto",
         encoding: str = None) -> Stage:
    """Load USD

    Args:
        file_like(Union[str, os.PathLike, IO[str], IO[bytes]]): File like object.
        format(str): Specify USD format.
        encoding(str): Optional. Explicitly specify encoding of the file.

    Returns:
        Stage. USD Stage.

    """

    from . import usd_loader

    usd = usd_loader.load(file_like, encoding=encoding, format=format)

    return usd


__all__ = ['Stage', 'version']
