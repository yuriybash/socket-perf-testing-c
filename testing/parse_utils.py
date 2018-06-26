import re, sys

MAX_NAME_LENGTH = 251

MAX_CONTENTS_LENGTH = 771

name_regex = re.compile ('^\s*(\w+)\s*$')

p1 = re.compile ('^([^:]+):(.*\n)$')

p2 = re.compile ('^\s+$')

command_regex = re.compile ('^\s*\\\\(\w+)(.*\n)$')

no_args = re.compile ('^\s*\n$')

one_arg = re.compile ('^\s*(\w+)\s*\n$')

two_args = re.compile ('^\s(\w+)\s+(\w+)\s*\n$')

def generate_names (filename):
    names = []
    f = open (filename, "r")
    for line in f:
        r1 = p1.match (line)
        if r1:
            n = r1.group (1)
            if len (n) > MAX_NAME_LENGTH:
                sys.stderr.write ("Name too long\n")
                sys.stderr.flush ()
                sys.exit (1)
            name_match = name_regex.match (n)
            if name_match:
                name = name_match.group (1)
            else:
                sys.stderr.write ("Name does not meet specification, " \
                        + "test invalid\n")
                sys.stderr.flush ()
                sys.exit (1)
            contents = r1.group (2)
            if len(contents) > MAX_CONTENTS_LENGTH:
                sys.stderr.write ("Contents are too long\n")
                sys.stderr.flush ()
                sys.exit (1)
            if name not in names:
                names.append (name)
    f.close ()
    if len (names) > 10:
        sys.stderr.write ("Error too many clients\n")
        sys.stderr.flush ()
        sys.exit (1)
    names.reverse ()
    return names
