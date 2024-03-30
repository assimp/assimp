from typing import Union

class Model():
    def __init__(self):
        pass

class Scope():
    def __init__(self):
        pass
class GPrim:
    def __init__(self):
        self.visibility = None
        self.proxyPrim = None

class Xform(GPrim):
    def __init__(self):
        self.xformOps = [] 

class Mesh(GPrim):
    def __init__(self):
        self.points = []

class Material():
    def __init__(self):
        pass

class Shader():
    def __init__(self):
        pass

class Camera(GPrim):
    def __init__(self):
        pass

class USDPath:
    """
    A struct to repersent Prim and property path.
    To distinguish pathlib.Path, Use USD prefix.

    TODO: variant
    """

    def __init__(self, prim_part: str = None, prop_part: str = None):
        self.prim_part = prim_part

        # NOTE: prefix '.' is omitted in property part.
        self.prop_part = prop_part

    def has_prim_part(self):
        if (self.prim_part is not None) and (len(self.prim_part) > 1):
            return True

    def has_prop_part(self):
        if (self.prop_part is not None) and (len(self.prop_part) > 1):
            return True

    def is_root_path(self):
        if self.prop_part is not None:
            return False

        # Empty str = root.
        if self.prim_part == "":
            return True

    def is_prop_path(self):
        """Test if a path is Prim property path
        """
        if self.is_root_path():
            return False

        # At the moment, Prim part must exist
        if self.prim_part is None:
            return False

        if self.prop_part is None:
            return False

        return True

    def is_prim_path(self):
        if self.prop_part is not None:
            return False

        if self.prim_part is None:
            return True

        return False

    def __str__(self):
        if self.is_root_path():
            return "/"

        if self.is_prim_path():
            return self.primp_part

        if self.has_prim_part() and self.has_prop_part():
            return self.prim_path + "." + self.prop_path

        elif self.has_prim_part():
            return self.prim_path

        elif self.has_prop_part():
            return self.prop_path
        else:
            # Invalid. return empty for now
            return ""


class TimeSampleData:
    def __init__(self, t: float, value):
        self.blocked: bool = False  # Value block
        self.t: float = t
        self.value = value

    def set_blocked(self):
        self.blocked = True

    def is_blocked(self):
        return self.blocked


class TimeSamples:
    def __init__(self, dtype: str):
        self.ts: dict = {}

    def set(self, t: float, value):
        self.ts[t] = TimeSampleData(t, value)

    def get(self, t: float):
        # TODO: interpolate time.
        return self.ts[t]

    def keys(self):
        self.ts.keys()

    def values(self):
        self.ts.values()


"""
NOTE:

"""


class PrimMeta(object):

    """
    Metadataum item

    - no property = not authored.
    - no ValueBlock(None)
    """

    _builtin_metadatum = {
            'active': bool,
            'sceneName': str,
        }

    _cnt = 0

    def __init__(self):
        # Do not register builtin metadataum

        print("init")
        self.__dict__["_custom_metadatum"] = {}
        print("init done")

        #self._builtin_metadataum()

    #def _builtin_metadataum(self):
    #    print("_builtin_metadatum fun")

    #    # dict of key: name, value: type
    #    d = {
    #        'active': bool,
    #        'sceneName': str,
    #    }

    #    return d

    def authored(self):
        PrimMeta._cnt = 2
        print(PrimMeta._cnt)

        """Test if Prim metadataum is authored(any metadatum is set)
        """
        for builtin in PrimMeta._builtin_metadatum:
            if builtin in self.__dict__:
                return True

        if len(self.__dict__["_custom_metadatum"]):
            return True

        return False

    def __setattr__(self, name, value):
        print("setattr name", name)
        print("setattr", self.__dict__)
        if name in PrimMeta._builtin_metadatum:
            print("builtin prim meta")
            ty = PrimMeta._builtin_metadatum[name]

            if not isinstance(value, ty):
                raise TypeError(
                    "Built-in metadatum `{}` is type of `{}`, but got type `{}` for the value.".format(
                        name, ty.__name__, type(value).__name__))

            self.__dict__[name] = value
        else:
            d = self.__dict__["_custom_metadatum"]
            print("custom metadataum", d)
            d[name] = value

    def __getattr__(self, name):
        print("PrimMeta getattr", name)
        print(self.authored())
        # use super() to avoid recurrent call of getattr
        if name in PrimMeta._builtin_metadataum:
            print("builtin")
            return super().__getattr__(name)

        print("getattr", name)
        return super().__getattr__("_custom_metadatum")[name]

    def __setitem__(self, key, value):
        print("setitem")
        # string key for now, e.g. PrimMeta["props:myval"]
        assert isinstance(key, str)

        self.__setattr__(key, value)

    def __getitem__(self, item):
        # string key for now, e.g. PrimMeta["props:myval"]
        assert isinstance(item, str)

        print("getitem")
        if item in PrimMeta._builtin_metadatum:
            if item in self.__dict__:
                return self.__dict__[item]
            
        custom_d = self.__dict__["_custom_metadatum"]
        if item in custom_d:
            return custom_d[item]

        raise AttributeError("{} not found.".format(item))
        

    def items(self):
        d = {}

        for builtin_name in PrimMeta._builtin_metadatum:
            if hasattr(self, builtin_name):
                d[builtin_name] = getattr(self, builtin_name)
        
        return d

class Prim:

    # TODO: support Union
    _builtin_props = {
        'name': (str, ""),
        'prim_type': (str, ""),
        'spec': (str, "def"),
        'element_name': (str, "")
    }

    def __init__(self, name: str = None, prim_type: Union[str, None] = None):

        # builtin props
        for (k, (ty, val)) in self._builtin_props.items():
            self.__dict__[k] = val

        # metadatum
        self.__dict__["_metas"] = PrimMeta()

        # custom props
        self.__dict__["_props"] = {}

    def __setattr__(self, name, value):
        if name in self._builtin_props:
            (ty, _) = self._builtin_props[name]
            if not isinstance(value, ty):
                raise TypeError(
                    "Built-in property `{}` is type of `{}`, but got type `{}` for the value.".format(
                        name, ty.__name__, type(value).__name__))

        print("setattr", name, value)
        self.__dict__["_props"][name] = value
        #self._props[name] = value

    def __getattr__(self, name):
        if name in self._builtin_props:
            print("builtin")
            return self.__dict__[name]

        print("getattr", name)
        return self.__dict__["_props"][name]

    def custom_props(self):
        return self.__dict__["_props"]

    def metas(self):
        return self.__dict__["_metas"]

    def __str__(self):
        s = "{} ".format(self.spec)
        if self.prim_type is not None:
            s += "{} ".format(self.prim_type)
        s += self.name
        s += " (\n"
        #self.metas().items()
        for (k, v) in self.metas().items():
            if v is not None:
                s += "  {} = {}\n".format(k, v)
        s += ") { \n"

        # print props
        for (k, v) in self.custom_props().items():
            s += "  " + str(k) + " " + str(v) + "\n"

        s += "}\n"

        return s

    def __repr__(self):

        return self.__str__()


#prim = Prim()
#prim.prim_type = "aa"
#prim.points = [1, 2, 3]

pmeta = PrimMeta()
#print("bora", pmeta.authored())
pmeta["userProperties:myval"] = 3
print(pmeta["userProperties:myval"])

#pmeta.bora = 1
#pmeta.active = True
#print("bora", pmeta.bora)
#print("active", pmeta.active)

# Prim metas
#prim.metas().active = False
#prim.name = "aa"
#prim.bora = "ss"

#print(prim)

#xform = Xform()