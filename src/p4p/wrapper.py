
from ._p4p import (Type as _Type, Value)

__all__ = (
    'Type',
    'Value',
    'Struct',
    'StructArray',
    'Union',
    'UnionArray',
)

def Struct(spec=None, id=None):
    return ('S', id, spec)

def Union(spec=None, id=None):
    return ('U', id, spec)

def StructArray(spec=None, id=None):
    return ('aS', id, spec)

def UnionArray(spec=None, id=None):
    return ('aU', id, spec)

class Type(_Type):
    __contains__ = _Type.has

    def __iter__(self):
        for k in self.keys():
            yield k

    def items(self):
        return [(k, self[k]) for k in self.keys()]

    def values(self):
        return [self[k] for k in self.keys()]

    def __repr__(self):
        S, id, fields = self.aspy()
        return 'Struct(%s, id="%s")'%(fields, id)
    __str__ = __repr__

_Type._magic(Type)
