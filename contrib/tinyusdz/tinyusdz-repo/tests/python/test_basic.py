import pytest

import tinyusdz
import typeguard

def test_token():
    a = tinyusdz.Token("bora")
    del a

def test_token_invalid_numeric():
    # raise error
    with pytest.raises(typeguard.TypeCheckError):
        a = tinyusdz.Token(1)

