# SPDX-License-Identifier: Apache 2.0
#
# USDA/USDZ/USDC loader using TinyUSDZ C API
#
# Requiement: Python 3.7+

import os
import warnings
from pathlib import Path
from typing import Union, IO

from . import Stage
from .compat_typing_extensions import TypeAlias  # python 3.10+

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
    from . import ctinyusd
    is_ctinyusd_available = True
except ImportError:
    is_ctinyusd_available = False


@typechecked
def is_usd(file_like: FILE_LIKE):
    if is_ctinyusd_available:

        if isinstance(file_like, str):
            ret = ctinyusd.c_tinyusd_is_usd_file(file_like)
            if ret == 0:
                return False
            else:
                return True

        elif isinstance(file_like, os.PathLike):
            # get string(filename) representation
            fspath = os.path.abspath(file_like)

            ret = ctinyusd.c_tinyusd_is_usd_file(fspath)
            if ret == 0:
                return False
            else:
                return True
        else:
            # Assume binary
            raise RuntimeError("TODO: Implement")

    else:
        warnings.warn(
            "Need `ctinyusd` native module to determine a given file is USD file.")
        return False


@typechecked
def load(file_like: FILE_LIKE,
         *,
         encoding: Union[str, None] = None,
         format: str = "usda") -> Stage:

    if isinstance(file_like, str):
        filepath = Path(file_like)

        if not filepath.exists():
            raise FileNotFoundError("File {} not found.".format(filepath))

        if not filepath.is_file():
            raise FileNotFoundError("File {} is not a file".format(filepath))

        fspath: str = os.path.abspath(filepath)

        warn_msg = ctinyusd.c_tinyusd_string_new_empty()
        err_msg = ctinyusd.c_tinyusd_string_new_empty()
        cstage = ctinyusd.c_tinyusd_stage_new()

        if format.lower() == "usda":
            ret: int = ctinyusd.c_tinyusd_load_usda_from_file(fspath, cstage, warn_msg, err_msg)
        elif format.lower() == "usdc":
            raise RuntimeError("TODO")
            pass
        elif format.lower() == "usdz":
            raise RuntimeError("TODO")
            pass
        else:
            raise RuntimeError("TODO")
            # auto detect
            pass

        ctinyusd.c_tinyusd_string_free(warn_msg)
        ctinyusd.c_tinyusd_string_free(err_msg)

        ctinyusd.c_tinyusd_stage_free(cstage)

    elif isinstance(file_like, os.PathLike):
        # get string(filename) representation
        filepath = Path(file_like)

        if not filepath.exists():
            raise FileNotFoundError("File {} not found.".format(filepath))

        if not filepath.is_file():
            raise FileNotFoundError("File {} is not a file".format(filepath))

        fspath: str = os.path.abspath(filepath)

        cwarn_msg = ctinyusd.c_tinyusd_string_new_empty()
        cerr_msg = ctinyusd.c_tinyusd_string_new_empty()
        cstage = ctinyusd.c_tinyusd_stage_new()

        if format.lower() == "usda":
            ret: int = ctinyusd.c_tinyusd_load_usda_from_file(fspath, cstage, cwarn_msg, cerr_msg)
            warn_msg = ctinyusd.c_tinyusd_string_str(cwarn_msg)
            err_msg = ctinyusd.c_tinyusd_string_str(cerr_msg)

            if warn_msg:
                warnings.warn("`{}`: {}", fspath, warn_msg)

            if ret == 0:
                raise RuntimeError("Failed to load USDA file `{}`: {}".format(fspath, err_msg))

        elif format.lower() == "usdc":
            raise RuntimeError("TODO")
            pass
        elif format.lower() == "usdz":
            raise RuntimeError("TODO")
            pass
        else:
            raise RuntimeError("TODO")
            # auto detect
            pass

        ctinyusd.c_tinyusd_string_free(warn_msg)
        ctinyusd.c_tinyusd_string_free(err_msg)

        ctinyusd.c_tinyusd_stage_free(cstage)

    else:
        # load from binary
        raise RuntimeError("TODO")

    warnings.warn("TODO: Implement")
    stage = Stage()
    return stage
