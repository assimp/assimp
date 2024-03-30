# SPDX-License-Identifier: Apache 2.0
#
# Requiement: Python 3.7+
#
# USD primitives

from typing import List, Any, Dict

# local modules
from .compat_typing_extensions import Literal, TypeAlias, TypedDict

try:
    from typeguard import typechecked
except ImportError:

    # no-op
    def typechecked(cls):
        return cls


SpecifierType: TypeAlias = Literal["def", "over", "class"]


@typechecked
class Prim:
    def __init__(self,
                 name: str,
                 specifier: SpecifierType = "def"):

        # assert specifier in Specifiers

        self._name: str = name

        # Corresponding Prim in C++ world.
        # self._prim = ctinyusdz.Prim()

        # Corresponding Prim index in C++ world.
        # 0 or None => Invalid
        self._prim_idx: int = 0

        self._specifier = specifier

        self._primChildren: List[Prim] = []

        # custom properties
        self._props: Dict = {}

        # metadatum
        self._metas: Dict = {}

        # TODO
        self._references = None
        self._payload = None
        self._variantSet: Dict = {}

    def specifier(self):
        return self._specifier

    def primChildren(self):
        return self._primChildren

    def set_prop(self, key: str, value: Any):
        self._props[key] = value

@typechecked
class Model(Prim):
    def __init__(self, name: str, specifier: str = "def", **kwargs):
        super().__init__(name, specifier)
        pass


@typechecked
class Scope(Prim):
    def __init__(self, name: str, specifier: str = "def", **kwargs):
        super().__init__(self, name, specifier)
        pass





@typechecked
class Xform(Prim):
    def __init__(self, name: str, specifier: str = "def"):
        super().__init__(self, specifier)

        # TODO: Typecheck
        self.xformOps = []


@typechecked
class GeomMesh(Prim):
    def __init__(self, name: str, specifier: str = "def"):
        super().__init__()
        pass


@typechecked
class Material(Prim):
    def __init__(self, name: str, specifier: str = "def"):
        pass


@typechecked
class Shader(Prim):
    def __init__(self):
        pass



@typechecked
class SkelRoot(Prim):
    def __init__(self, name: str, specifier: str = "def", **kwargs):
        super().__init__(self, name, specifier)
        # TODO
        pass


@typechecked
class SkelAnimtion(Prim):
    def __init__(self, name: str, specifier: str = "def", **kwargs):
        super().__init__(self, name, specifier)
        # TODO
        pass


@typechecked
class Skeleton(Prim):
    def __init__(self, name: str, specifier: str = "def", **kwargs):
        super().__init__(self, name, specifier)
        # TODO
        pass


@typechecked
class BlendShape(Prim):
    def __init__(self, name: str, specifier: str = "def", **kwargs):
        super().__init__(self, name, specifier)

        self.offsets = None
        self.normalOffsets = None
        self.pointIndices = None
        # TODO
        pass

@typechecked
class UsdPreviewSurface(Shader):
    def __init__(self):
        super().__init__(self)

        self.diffuseColor = [0.18, 0.18, 0.18]
        # TODO: More attrs


