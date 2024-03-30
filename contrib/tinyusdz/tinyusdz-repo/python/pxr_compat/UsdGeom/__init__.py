from . import Tokens

def SetStageUpAxis(cls, stage: Stage, axis: Tokens):
    assert axis == Tokens.x or axis == Tokens.y or axis == Tokens.z
    stage.upAxis = axis

