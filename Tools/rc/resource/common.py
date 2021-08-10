#
# Common functions and definitions
#

import os, sys, json, platform
from collections import OrderedDict

sys.path.insert(1, os.path.expandvars('${SMING_HOME}/../Tools/Python'))
from rjsmin import jsmin

quiet = False

def status(msg):
    """ Print status message to stderr """
    if not quiet:
        critical(msg)


def critical(msg):
    """ Print critical message to stderr """
    sys.stderr.write(msg)
    sys.stderr.write('\n')


def compact_string(s):
    return ''.join(s.split())


def json_loads(s):
    return json.loads(jsmin(s), object_pairs_hook=OrderedDict)

def json_load(filename):
    with open(filename) as f:
        return json_loads(f.read())

def json_save(data, filename):
    with open(filename, "w") as f:
        json.dump(data, f, indent=4)

def to_json(obj):
    return json.dumps(obj, indent=4)


class InputError(RuntimeError):
    def __init__(self, e):
        super(InputError, self).__init__(e)


class ValidationError(InputError):
    def __init__(self, obj, message):
        super(ValidationError, self).__init__("%s.%s '%s' invalid: %s" % (type(obj).__module__, type(obj).__name__, obj.name, message))
        self.obj = obj
