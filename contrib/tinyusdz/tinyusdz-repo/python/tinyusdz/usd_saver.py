# SPDX-License-Identifier: Apache 2.0
#
# USDA/USDZ/USDC loader using TinyUSDZ C API
#
# Requiement: Python 3.7+

import os
from typing import Union, IO

from tinyusdz import dumps
from tinyusdz import Stage

#
# try:
#    import ctinyusd
#    is_ctinyusd_available = True
# except ImportError:
#    is_ctinyusd_available = False


def save(stage: Stage,
         file_like: Union[str, os.PathLike, IO[str], IO[bytes]],
         *,
         format: str = "usda",
         indent: int = 2) -> None:

    if isinstance(file_like, str):
        # filename
        f = open(file_like, 'w', encoding='utf-8')
    else:
        f = file_like

    s: str = dumps(stage, format=format, indent=indent)

    f.write(s)

    f.close()

    return True
