from collections import defaultdict

from parse_utils import MAX_NAME_LENGTH, MAX_CONTENTS_LENGTH, name_regex, p1, p2, command_regex, no_args, one_arg, two_args, generate_names

import re, sys, os, argparse, signal

prefix = "\r     \r"

suffix = "[Me]:"

keep_blank = False

mem_check = False

class Command:
    def __init__ (self, name, args, msg):
        self.args = args
        self.name = name
        self.msg = msg

    def handle_command (self, names, file_dict):
        file_dict[self.name].write ("{}Unknown command {}\n{}".format (prefix, self.msg, suffix))

    def handle_wrong_args (self, file_dict):
        file_dict[self.name].write ("{}Incorrect arguments for {} command\n{}".format (prefix, self.msg, suffix))
        
    def handle_input (self, line_num, names, names_dict):
        self.handle_wrong_input (line_num, names, names_dict)

    def handle_wrong_input (self, line_num, names, names_dict):
        names_dict[self.name].append("1\n")
        for n in names:
            names_dict[n].append ("S {}\n".format (line_num))



class Exit (Command):
    def handle_command (self, names, file_dict):
        arg_match = no_args.match (self.args)
        if arg_match:
            actual_name = names[self.name].name
            del names[self.name]
            for n in names.keys():
                file_dict[n].write ("{}{} has left\n{}".format (prefix, actual_name, suffix))
            file_dict[self.name].write ("{}You have left\n".format (prefix))
        else:
            self.handle_wrong_args (file_dict)

    def handle_input (self, line_num, names, names_dict):
        arg_match = no_args.match (self.args)
        if arg_match:
            for n in names:
                names_dict[n].append ("1\n")
            del names[self.name]
        else:
            self.handle_wrong_input (line_num, names, names_dict)


class Server_Exit (Command):
    def handle_command (self, names, file_dict):
        arg_match = no_args.match (self.args)
        if arg_match:
            name_list = [n for n in names.keys ()]
            for n in name_list:
                file_dict[n].write ("{}Server has disconnected\n".format (prefix))
                del names[n]
        else:
            self.handle_wrong_args (file_dict)

    def handle_input (self, line_num, names, names_dict):
        arg_match = no_args.match (self.args)
        if arg_match:
            name_list = [n for n in names.keys ()]
            for n in name_list:
                names_dict[n].append ("1\n")
                del names[n]
        else:
            self.handle_wrong_input (line_num, names, names_dict)

class Set_Nickname (Command):
    def handle_command (self, names, file_dict):
        arg_match = two_args.match (self.args)
        if arg_match:
            old_name = arg_match.group (1)
            name_dict = {value.name : name  for name, value in names.items ()}
            if old_name not in name_dict:
                self.handle_wrong_args (file_dict)
                return
            new_name = arg_match.group (2)
            if len(new_name) > MAX_NAME_LENGTH:
                self.handle_wrong_args (file_dict)
                return
            if new_name in name_dict:
                self.handle_wrong_args (file_dict)
                return
            nicknames = [x.nickname for x in names.values ()]
            if new_name in nicknames:
                self.handle_wrong_args (file_dict)
                return
            names[name_dict[old_name]].set_nickname (new_name)
            if old_name == names[self.name].name:
                file_dict[self.name].write ("{}You set your nickname to {}\n{}"
                        .format(prefix, new_name, suffix))
            else:
                file_dict[self.name].write ("{}You set {}'s nickname to {}\n{}"
                        .format(prefix, old_name, new_name, suffix))
            for n in names:
                if n != self.name:
                    file_dict[n].write ("{}{} set {}'s nickname to {}\n{}".format
                        (prefix, names[self.name].name, old_name, new_name, suffix))
        else:
           self.handle_wrong_args (file_dict)

    def handle_input (self, line_num, names, names_dict):
        arg_match = two_args.match (self.args)
        if arg_match:
            old_name = arg_match.group (1)
            name_dict = {value.name : name  for name, value in names.items ()}
            if old_name not in name_dict:
                self.handle_wrong_input (line_num, names, names_dict)
                return
            new_name = arg_match.group (2)
            if len(new_name) > MAX_NAME_LENGTH:
                self.handle_wrong_input (line_num, names, names_dict)
                return
            if new_name in name_dict:
                self.handle_wrong_input (line_num, names, names_dict)
                return
            nicknames = [x.nickname for x in names.values ()]
            if new_name in nicknames:
                self.handle_wrong_input (line_num, names, names_dict)
                return
            names[name_dict[old_name]].set_nickname (new_name)
            for n in names:
                names_dict[n].append ("1\n")
        else:
            self.handle_wrong_input (line_num, names, names_dict)


class Clear_Nickname (Command):
    def handle_command (self, names, file_dict):
        arg_match = one_arg.match (self.args)
        if arg_match:
            old_name = arg_match.group (1)
            name_dict = {value.name : name for name, value in names.items ()}
            if old_name not in name_dict:
                self.handle_wrong_args (file_dict)
                return
            names[name_dict[old_name]].clear_nickname ()
            if old_name == names[self.name].name:
                file_dict[self.name].write ("{}You have cleared your nickname\n{}".format (prefix, suffix))
            else:
                file_dict[self.name].write ("{}You have cleared {}'s nickname\n{}".format (prefix, old_name, suffix))
            for n in names:
                if n != self.name:
                    file_dict[n].write ("{}{} has cleared {}'s nickname\n{}".format
                            (prefix, names[self.name].name, old_name, suffix))
        else:
            self.handle_wrong_args (file_dict)

    def handle_input (self, line_num, names, names_dict):
        arg_match = one_arg.match (self.args)
        if arg_match:
            old_name = arg_match.group (1)
            name_dict = {value.name : name for name, value in names.items ()}
            if old_name not in name_dict:
                self.handle_wrong_input (line_num, names, names_dict)
                return
            names[name_dict[old_name]].clear_nickname ()
            for n in names:
                names_dict[n].append ("1\n")
        else:
            self.handle_wrong_input (line_num, names, names_dict)


class Rename (Command):
    def handle_command (self, names, file_dict):
        arg_match = one_arg.match (self.args)
        if arg_match:
            new_name = arg_match.group (1)
            name_list = [name.name for name in names.values ()]
            if new_name in name_list:
                self.handle_wrong_args (file_dict)
                return
            nickname_list = [name.nickname for name in names.values ()]
            if new_name in nickname_list:
                self.handle_wrong_args (file_dict)
                return
            if len(new_name) > MAX_NAME_LENGTH:
                self.handle_wrong_args (file_dict)
                return
            old_name = names[self.name].name
            names[self.name].rename (new_name)
            for n in names:
                if n != self.name:
                    file_dict[n].write ("{}{} changed their name to {}\n{}".format 
                        (prefix, old_name, new_name, suffix))
            file_dict[self.name].write ("{}You have changed your name to {}\n{}"
                    .format(prefix, new_name, suffix))
        else:
            self.handle_wrong_args (file_dict)

    def handle_input (self, line_num, names, names_dict):
        arg_match = one_arg.match (self.args)
        if arg_match:
            new_name = arg_match.group (1)
            name_list = [name.name for name in names.values ()]
            if new_name in name_list:
                self.handle_wrong_input (line_num, names, names_dict)
                return
            nickname_list = [name.nickname for name in names.values ()]
            if new_name in nickname_list:
                self.handle_wrong_input (line_num, names, names_dict)
                return
            if len(new_name) > MAX_NAME_LENGTH:
                self.handle_wrong_input (line_num, names, names_dict)
                return
            names[self.name].rename (new_name)
            for n in names:
                names_dict[n].append ("1\n")
        else:
            self.handle_wrong_input (line_num, names, names_dict)

class Mute (Command):
    def handle_command (self, names, file_dict):
        arg_match = one_arg.match (self.args)
        if arg_match:
            name = arg_match.group (1)
            name_dict = {value.name : name for name, value in names.items ()}
            if name not in name_dict:
                self.handle_wrong_args (file_dict)
            elif names[self.name].name == name:
                self.handle_wrong_args (file_dict)
            else:
                names[self.name].mute (name_dict[name])
                file_dict[self.name].write ("{}User {} is now muted\n{}".format
                        (prefix, name, suffix))
        else:
            self.handle_wrong_args (file_dict)

    def handle_input (self, line_num, names, names_dict):
        arg_match = one_arg.match (self.args)
        if arg_match:
            name = arg_match.group (1)
            name_dict = {value.name : name for name, value in names.items ()}
            if name not in name_dict:
                self.handle_wrong_input (line_num, names, names_dict)
            elif names[self.name].name == name:
                self.handle_wrong_input (line_num, names, names_dict)
            else:
                names[self.name].mute (name_dict[name])
                self.handle_wrong_input (line_num, names, names_dict)
        else:
            self.handle_wrong_input (line_num, names, names_dict)


class Unmute (Command):
    def handle_command (self, names, file_dict):
        arg_match = one_arg.match (self.args)
        if arg_match:
            name = arg_match.group (1)
            name_dict = {value.name : name for name, value in names.items ()}
            if name not in name_dict:
                self.handle_wrong_args (file_dict)
            elif name == names[self.name].name:
                self.handle_wrong_args (file_dict)
            else:
                names[self.name].unmute (name_dict[name])
                file_dict[self.name].write ("{}User {} is no longer muted\n{}"
                        .format(prefix, name, suffix))
        else:
            self.handle_wrong_args (file_dict)

    def handle_input (self, line_num, names, names_dict):
        arg_match = one_arg.match (self.args)
        if arg_match:
            name = arg_match.group (1)
            name_dict = {value.name : name for name, value in names.items ()}
            if name not in name_dict:
                self.handle_wrong_input (line_num, names, names_dict)
            elif name == names[self.name].name:
                self.handle_wrong_input (line_num, names, names_dict)
            else:
                names[self.name].unmute (name_dict[name])
                self.handle_wrong_input (line_num, names, names_dict)
        else:
            self.handle_wrong_input (line_num, names, names_dict)


class Show_Status (Command):
    def handle_command (self, names, file_dict):
        arg_match = one_arg.match (self.args)
        if arg_match:
            new_name = arg_match.group (1)
            name_dict = {value.name : name for name, value in names.items ()}
            if new_name not in name_dict:
                self.handle_wrong_args (file_dict)
                return
            name = name_dict[new_name]
            self.output_status_info (file_dict, name, names)
        else:
            self.handle_wrong_args (file_dict)

    def output_status_info (self, file_dict, new_name, names):
        if names[new_name].has_nickname:
            second_string = "{}Nickname {}\n{}".format (prefix, names[new_name].nickname, suffix)
        else:
            second_string = "{}User has no nickname\n{}".format (prefix, suffix)
        if new_name in names[self.name].mutelist:
            third_string = "{}User is muted\n{}".format (prefix, suffix)
        else:
            third_string = "{}User is not muted\n{}".format (prefix, suffix)
        file_dict[self.name].write ("{}User {}\n{}{}{}".format (prefix, names[new_name].name, suffix
            , second_string, third_string))

    def handle_input (self, line_num, names, names_dict):
        arg_match = one_arg.match (self.args)
        if arg_match:
            new_name = arg_match.group (1)
            name_dict = {value.name : name for name, value in names.items ()}
            if new_name not in name_dict:
                self.handle_wrong_input (line_num, names, names_dict)
                return
            names_dict[self.name].append ("{}\n".format (3))
            for n in names:
                names_dict[n].append ("S {}\n".format (line_num))
        else:
            self.handle_wrong_input (line_num, names, names_dict)


class Show_All_Statuses (Show_Status):
    def handle_command (self, names, file_dict):
        arg_match = no_args.match (self.args)
        if arg_match:
            names_dict = {value.name : name for name, value in names.items ()}
            names_list = list (names_dict.keys ())
            names_list.sort ()
            for new_name in names_list:
                self.output_status_info (file_dict, names_dict[new_name], names)
        else:
            self.handle_wrong_args (file_dict)

    def handle_input (self, line_num, names, names_dict):
        arg_match = no_args.match (self.args)
        if arg_match:
            names_dict[self.name].append ("{}\n".format (3 * len (names.keys ())))
            for n in names:
                names_dict[n].append ("S {}\n".format (line_num))
        else:
            self.handle_wrong_input (line_num, names, names_dict)


command_messages = defaultdict (lambda : Command)

command_contents = [("exit", Exit), ("server_exit", Server_Exit), 
    ("set_nickname", Set_Nickname), ("clear_nickname", Clear_Nickname), 
    ("rename", Rename), ("mute", Mute), ("unmute", Unmute), 
    ("show_status", Show_Status), ("show_all_statuses", Show_All_Statuses)]

for k, v in command_contents:
    command_messages[k] = v


class UserInfo ():
    def __init__ (self, name):
        self.name = name
        self.nickname = name
        self.has_nickname = False
        self.mutelist = set()

    def set_nickname (self, new_name):
        self.nickname = new_name
        self.has_nickname = True

    def clear_nickname (self):
        self.nickname = self.name
        self.has_nickname = False

    def rename (self, new_name):
        self.name = new_name
        if not self.has_nickname:
            self.nickname = new_name

    def mute (self, name):
        self.mutelist.add (name)

    def unmute (self, name):
        self.mutelist.discard (name)

def generate_inputs (dictionary, filename, names_list):
    dir_name = "{}_inputs".format (filename)
    for name, lines in dictionary.items ():
        f = open ("{}/{}.in".format (dir_name, name), "w")
        for line in lines:
            f.write (line)
        f.flush ()
        f.close ()
    f = open ("{}/nameslist".format (dir_name), "w")
    for name in names_list:
        f.write ("{}\n".format (name));
    f.flush ()
    f.close ()

def produce_inputs (names, filename):
    names_dict = dict()
    for index, name in enumerate(names):
        if index < len(names) - 1:
            names_dict[name] = ["{}\n".format ((len(names) - 1) - index)]
        else:
            names_dict[name] = []
    actual_line = 2 + len (names)
    names = {name : UserInfo (name) for name in names}
    name = None
    contents = None
    f = open (filename, "r")
    for line in f:
        r1 = p1.match (line)
        if r1:
            n = r1.group (1)
            name_match = name_regex.match (n)
            if name_match:
                name = name_match.group (1)
                if name not in names:
                    continue
            else:
                continue
            contents = r1.group (2)
            command = parse_command (name, contents)
            names_dict[name].append ("->{}".format (contents))
            if command:
                command.handle_input (actual_line, names, names_dict)
            else:
                for n in names:
                    if name != n:
                        if name not in names[n].mutelist:
                            names_dict[n].append ("1\n")
                        else:
                            names_dict[n].append ("S {}\n".format (actual_line))
            actual_line += 1
        elif keep_blank:
            if name:
                r2 = p2.match (line)
                if r2:
                    contents = r2.group (0)
                    names_dict[name].append ("->{}".format (contents))
                    for n in names:
                        if name != n:
                            if name not in names[n].mutelist:
                                names_dict[n].append ("1\n")
                            else:
                                names_dict[n].append ("S {}\n".format (actual_line))
                    actual_line += 1
    f.close ()
    if mem_check:
            names_remaining = [n for n in names.keys ()]
            if len (names_remaining) > 0:
                names_remaining.sort ()
                names_dict[names_remaining[0]].append ("->\server_exit\n")
                for n in names_remaining:
                    names_dict[n].append ("1\n")
    return names_dict

def generate_outputs (names, filename):
    dir_name = "{}_refs".format (filename)
    input_dir = "{}_inputs".format (filename)
    if not os.path.exists (dir_name):
        os.makedirs (dir_name)
    if not os.path.exists (input_dir):
        os.makedirs (input_dir)
    server_file = open ("{}/server".format (dir_name), "w")
    server_file.write ("Server messages:\n")
    for name in names:
        server_file.write ("{} has joined\n".format (name))
    file_dict = dict ()
    for name in names:
        file_dict[name] = open ("{}/{}.ref".format (dir_name, name), "w")
        file_dict[name].write ("{}".format (suffix))
    length = len (names)
    for index, name in enumerate(names):
        for i in range (index + 1, length):
            file_dict[name].write("{}{} has joined\n{}".format (prefix, names[i], suffix))
    names = {name : UserInfo (name) for name in names}
    name = None
    contents = None
    f = open (filename, "r")
    for line in f:
        r1 = p1.match (line)
        if r1:
            n = r1.group (1)
            name_match = name_regex.match (n)
            if name_match:
                name = name_match.group (1)
                if name not in names:
                    continue
            else:
                continue
            contents = r1.group (2)
            server_file.write (contents)
            command = parse_command (name, contents)
            file_dict[name].write ("{}".format (suffix))
            if command:
                command.handle_command (names, file_dict)
            else:
                for n in names:
                    if name != n and name not in names[n].mutelist:
                        file_dict[n].write ("{}{}:{}{}".format (prefix, names[name].nickname, contents, suffix))
        elif keep_blank:
            if name:
                r2 = p2.match (line)
                if r2:
                    contents = r2.group (0)
                    server_file.write (contents)
                    for n in names:
                        if name == n:
                            file_dict[n].write ("{}".format (suffix))
                        else:
                            file_dict[n].write ("{}{}:{}{}".format (prefix, names[name].nickname, contents, suffix))
    process_remaining (names, file_dict, input_dir)
    f.close ()
    if mem_check and len (names.keys ()) > 0:
        server_file.write ("\server_exit\n")
    server_file.flush ()
    server_file.close ()
    for name in file_dict.keys ():
        f = file_dict[name]
        f.flush ()
        f.close ()


def parse_command (name, contents):
    command = command_regex.match (contents)
    if command:
        command_name = command.group (1)
        args = command.group (2)
        return command_messages[command_name] (name, args, command_name)
    else:
        return None


def process_remaining (names, file_dict, input_dir):
    remaining = list (names.keys ())
    remaining.sort ()
    length = len (remaining)
    f = open ("{}/remaining_names".format (input_dir), "w+")
    if mem_check:
        if length > 0:
            file_dict[remaining[0]].write ("{}".format (suffix))
            for name in remaining:
                file_dict[name].write ("{}Server has disconnected\n".format (prefix))
    else:
        for i, name in enumerate (remaining):
            f.write ("{}\n".format (name))
            for j in range (i + 1, length):
                file_dict[remaining[j]].write ("{}{} has left\n{}".format (prefix, names[name].name, suffix))
    f.flush ()
    f.close ()


def main (filename):
    names = generate_names (filename)
    generate_outputs (names, filename)
    dictionary = produce_inputs ([n for n in names], filename)
    generate_inputs (dictionary, filename, names)


def handler (signum, frame):
    print "Exiting due to interupt"
    exit (1)

if __name__ == '__main__':
    signal.signal (signal.SIGUSR1, handler)
    parser = argparse.ArgumentParser (description="Parse an input file to \
            create deterministic tests for clients in a chatroom of max \
            size 10 clients.")
    parser.add_argument ("--w", dest="w", action="store_const", const=True,
            default=False, help="Enable files to extend the whitespace \
                    after an existing message to be part of the message \
                    being sent.")
    parser.add_argument ("--m", dest="m", action="store_const", const=True,
            default=False, help="Enable checks for memory by causes any \
            the first of any remaining names to call the server exit command.")
    parser.add_argument ("filename", type=str,
            help="A file to read as the list of messages to be sent.")
    items = parser.parse_args ()
    keep_blank = items.w
    mem_check = items.m
    main (items.filename)
