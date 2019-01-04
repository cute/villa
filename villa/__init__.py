# -*- encoding:utf-8 -*-

from . import villa

class Villa(object):
    """docstring for Villa"""

    def __init__(self, path, mode):
        super(Villa, self).__init__()
        self.db = villa.open(path, mode)

    def iter(self, prefix):
        for key, value in self.db.iterprefix(prefix, villa.VL_JFORWARD):
            if not key.startswith(prefix):
                break
            yield key, value

    def truncate(self, prefix):
        self.db.trunprefix(prefix, villa.VL_JFORWARD)

    def push(self, key, value):
        self.db.put(key, value, villa.VL_DDUP)

    def set(self, key, value):
        self.db.put(key, value, villa.VL_DOVER)

    def delete(self, key, value):
        del self.db[key]

    def get(self, key):
        return self.db.get(key)

    def getlist(self, key):
        return self.db.getlist(key)

    def __setitem__(self, key, value):
        self.db.put(key, value, villa.VL_DOVER)

    def __getitem__(self, key):
        return self.db[key]

    def __delitem__(self, key):
        del self.db[key]

    def __getattr__(self, key):
        if hasattr(self.db, key):
            return getattr(self.db, key)
        else:
            return None

    def __del__(self):
        self.db.close()
