from tree_walker import TreeWalker
from libyang.schema import Node as LyNode
from callback import Callback
from utils import to_c_variable

class RPCContext:
    def __init__(self):
        self.callbacks = []
        self.types = {
            "unknown": "SR_UNKNOWN_T",
            "binary": "SR_BINARY_T",
            "uint8": "SR_UINT8_T",
            "uint16": "SR_UINT16_T",
            "uint32": "SR_UINT32_T",
            "uint64": "SR_UINT64_T",
            "string": "SR_STRING_T",
            "bits": "SR_BITS_T",
            "boolean": "SR_BOOL_T",
            "decimal64": "SR_DECIMAL64_T",
            "empty": "SR_LEAF_EMPTY_T",
            "enumeration": "SR_ENUM_T",
            "identityref": "SR_IDENTITY_REF_T",
            "instance-id": "SR_INSTANCEID_T",
            "leafref": "SR_UNKNOWN_T",
            "union": "SR_UNKNOWN_T",
            "int8": "SR_INT8_T",
            "int16": "SR_INT16_T",
            "int32": "SR_INT32_T",
            "int64": "SR_INT64_T",
        }

        self.struct_types = {
            "unknown": "",
            "binary": "binary_val",
            "uint8": "uint8_val",
            "uint16": "uint16_val",
            "uint32": "uint32_val",
            "uint64": "uint64_val",
            "string": "string_val",
            "bits": "bits_val",
            "boolean": "bool_val",
            "decimal64": "decimal64_val",
            "empty": "",
            "enumeration": "enum_val",
            "identityref": "identityref_val",
            "instance-id": "instanceid_val",
            "leafref": "",
            "union": "",
            "int8": "int8_val",
            "int16": "int16_val",
            "int32": "int32_val",
            "int64": "int64_val",
        }


class Walker(TreeWalker):
    def __init__(self, prefix, root_nodes):
        super().__init__(root_nodes)
        self.ctx = RPCContext()

    def walk_node(self, node, depth):
        # print("\t" * depth, end="")
        # print("{}[{}]".format(node.name(), node.keyword()))

        input_nodes = []
        nodes = list(node.input().children())
        while nodes:
            n = nodes.pop()
            if n.nodetype() in [LyNode.LIST, LyNode.CONTAINER]:
                nodes.extend(n.children())
            else:
                input_nodes.append(n)

        output_nodes = []
        nodes = list(node.output().children())
        while nodes:
            n = nodes.pop()
            if n.nodetype() in [LyNode.LIST, LyNode.CONTAINER]:
                nodes.extend(n.children())
            else:
                output_nodes.append(n)

        self.ctx.callbacks.append((Callback(node.data_path(),
                                           to_c_variable(node.name())), (input_nodes, output_nodes)))

        return depth > -1

    def add_node(self, node):
        return node.nodetype() == LyNode.RPC

    def get_callbacks(self):
        return list(reversed(self.ctx.callbacks))

    def get_types(self):
        return self.ctx.types

    def get_struct_types(self):
        return self.ctx.struct_types
