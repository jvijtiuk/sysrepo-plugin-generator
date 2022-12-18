from tree_walker import TreeWalker
import os
from libyang.schema import Node as LyNode

from utils import to_c_variable


class APIContext:
    def __init__(self, prefix, source_dir):
        self.source_dir = source_dir
        self.files = ['check', 'load', 'store']
        self.notif_files = ['notif']
        self.extensions = ['c', 'h']

        self.dirs_stack = {
            0: os.path.join(source_dir, "plugin", "api")
        }

        self.prefix_stack = {
            0: ''
        }

        self.notif_prefix_stack = {
                0: prefix + "/",
        }

        self.types = {
            "unknown": "void *",
            "binary": "void *",
            "uint8": "uint8_t",
            "uint16": "uint16_t",
            "uint32": "uint32_t",
            "uint64": "uint64_t",
            "string": "char *",
            "bits": None,
            "boolean": "uint8_t",
            "decimal64": "double",
            "empty": "void",
            "enumeration": "char *",
            "identityref": "char *",
            "instance-id": None,
            "leafref": None,
            "union": "void *",
            "int8": "int8_t",
            "int16": "int16_t",
            "int32": "int32_t",
            "int64": "int64_t",
        }

        self.dir_functions = {}
        self.notifs = {}
        self.notif_dir = os.path.join(source_dir, "plugin", "api")

        # iterate tree and create directories
        self.dirs = []


class Walker(TreeWalker):
    def __init__(self, prefix, root_nodes, source_dir):
        super().__init__(root_nodes)
        self.ctx = APIContext(prefix, source_dir)

    def walk_node(self, node, depth):
        # print("\t" * depth, end="")
        # print("{}[{}]".format(node.name(),
        #       node.keyword()))

        last_path = self.ctx.dirs_stack[depth]
        last_prefix = self.ctx.prefix_stack[depth]
        last_notif_prefix = self.ctx.notif_prefix_stack[depth]

        if node.nodetype() == LyNode.CONTAINER:
            if last_path not in self.ctx.dir_functions:
                self.ctx.dir_functions[last_path] = (last_prefix[:-1], [])

            # update dir stack
            new_dir = os.path.join(last_path, node.name())
            self.ctx.dirs_stack[depth +
                                1] = new_dir
            self.ctx.dirs.append(new_dir)

            # update prefix stack
            new_prefix = last_prefix + to_c_variable(node.name()) + "_"
            new_notif_prefix = last_notif_prefix + to_c_variable(node.name()) + "_"
            self.ctx.prefix_stack[depth + 1] = new_prefix
            self.ctx.notif_prefix_stack[depth + 1] = new_notif_prefix

        if node.nodetype() == LyNode.LIST:
            if last_path not in self.ctx.dir_functions:
                self.ctx.dir_functions[last_path] = (last_prefix[:-1], [])

            self.ctx.dir_functions[last_path][1].append(node)

            # update dir stack
            new_dir = os.path.join(last_path, node.name())
            self.ctx.dirs_stack[depth +
                                1] = new_dir
            self.ctx.dirs.append(new_dir)

            # update prefix stack
            new_prefix = last_prefix + to_c_variable(node.name()) + "_"
            new_notif_prefix = last_notif_prefix + to_c_variable(node.name()) + "_"
            self.ctx.prefix_stack[depth + 1] = new_prefix
            self.ctx.notif_prefix_stack[depth + 1] = new_notif_prefix

        if node.nodetype() == LyNode.LEAF or node.nodetype() == LyNode.LEAFLIST:
            if last_path not in self.ctx.dir_functions:
                self.ctx.dir_functions[last_path] = (last_prefix[:-1], [])

            self.ctx.dir_functions[last_path][1].append(node)
            return True

        if node.nodetype() == LyNode.NOTIF:
            if last_path not in self.ctx.notifs:
                self.ctx.notifs[last_path] = (last_notif_prefix[:-1], [])

            self.ctx.notifs[last_path][1].append(node)
            return True

        return False

    def add_node(self, node):
        return not node.config_false() and not node.nodetype() in [LyNode.RPC]

    def get_directories(self):
        return self.ctx.dirs

    def get_notif_directories(self):
        dirs = self.ctx.dirs
        dirs.append(self.ctx.notif_dir)
        return dirs

    def get_directory_functions(self):
        return self.ctx.dir_functions

    def get_notifs(self):
        return self.ctx.notifs

    def get_api_filenames(self):
        files = []

        for fn in self.ctx.files:
            for ext in self.ctx.extensions:
                files.append("{}.{}".format(fn, ext))
        return files

    def get_notif_filenames(self):
        files = []

        for fn in self.ctx.notif_files:
            for ext in self.ctx.extensions:
                files.append("{}.{}".format(fn, ext))
        return files

    def get_types(self):
        return self.ctx.types
