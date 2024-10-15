import os
import sys

module_format = """// register_module_types.gen.cpp
/* THIS FILE IS GENERATED DO NOT EDIT */
#include "register_module_types.h"

#include "modules/modules_enabled.gen.h"

%s

void register_module_types() {
%s
}

void unregister_module_types() {
%s
}
""" 

def write_modules(modules):
    includes_cpp = ""
    register_cpp = ""
    unregister_cpp = ""

    for name in modules:
        path = "modules/"+name
        includes_cpp += '#include "' + path + '/register_types.h"\n'
        register_cpp += "#ifdef MODULE_" + name.upper() + "_ENABLED\n"
        register_cpp += "\tregister_" + name + "_types();\n"
        register_cpp += "#endif\n"
        unregister_cpp += "#ifdef MODULE_" + name.upper() + "_ENABLED\n"
        unregister_cpp += "\tunregister_" + name + "_types();\n"
        unregister_cpp += "#endif\n"

    modules_cpp = module_format % (
        includes_cpp,
        register_cpp,
        unregister_cpp,
    )

    # NOTE: It is safe to generate this file here, since this is still executed serially
    with open("register_module_types.gen.cpp", "w") as f:
        f.write(modules_cpp)

def generate_modules_enabled(target, modules):
    with open(target, "w") as f:
        for module in modules:
            f.write("#define " + "MODULE_" + module.upper() + "_ENABLED\n")

if __name__ == "__main__":
    modules = sys.argv[1:]
    write_modules(modules)
    generate_modules_enabled("modules_enabled.gen.h", modules)
    print("%d modules generated" % len(modules))