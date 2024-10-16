import codecs
from collections import OrderedDict
import glob
import os
import sys

def gen_docs():
    def gather_modules():
        modules = OrderedDict()
        def is_module(path):
            if not os.path.isdir(path):
                return False
            must_exist = ["register_types.h", "CMakeLists.txt", "config.py"]
            for f in must_exist:
                if not os.path.exists(os.path.join(path, f)):
                    return False
            return True

        def get_files(path):
            files = glob.glob(os.path.join(path, "*"))
            # Sort so that `register_module_types` does not change that often,
            # and plugins are registered in alphabetic order as well.
            files.sort()
            return files

        def add_module(path):
            module_name = os.path.basename(path)
            module_path = path.replace("\\", "/")  # win32
            modules[module_name] = module_path

        for c in get_files("../modules"):
            if is_module(c):
                add_module(c)

        return modules

    # Core API documentation.
    docs = []
    docs += glob.glob("../doc/classes/*.xml")

    # Module API documentation.
    doc_modules = gather_modules()
    module_dirs = []
    for d in doc_modules.values():
        if d not in module_dirs:
            module_dirs.append(d)

    for d in module_dirs:
        if not os.path.isabs(d):
            docs += glob.glob("../" + d + "/*.xml")  # Built-in.
        else:
            docs += glob.glob(d + "/*.xml")  # Custom.

    with open("doc_data_class_path.gen.h", "w", encoding="utf-8") as g:
        g.write("static const int _doc_data_class_path_count = " + str(len(doc_modules)) + ";\n")
        g.write("struct _DocDataClassPath { const char* name; const char* path; };\n")

        g.write("static const _DocDataClassPath _doc_data_class_paths[" + str(len(doc_modules) + 1) + "] = {\n")
        for c in sorted(doc_modules):
            g.write('\t{"' + c + '", "' + doc_modules[c] + '"},\n')
        g.write("\t{NULL, NULL}\n")
        g.write("};\n")

    def make_doc_header(dst, sources):
        g = open(dst, "w", encoding="utf-8")
        buf = ""
        docbegin = ""
        docend = ""
        for src in sources:
            if not src.endswith(".xml"):
                continue
            with open(src, "r", encoding="utf-8") as f:
                content = f.read()
            buf += content

        buf = codecs.utf_8_encode(docbegin + buf + docend)[0]
        decomp_size = len(buf)
        import zlib

        buf = zlib.compress(buf)

        g.write("/* THIS FILE IS GENERATED DO NOT EDIT */\n")
        g.write("#ifndef _DOC_DATA_RAW_H\n")
        g.write("#define _DOC_DATA_RAW_H\n")
        g.write("static const int _doc_data_compressed_size = " + str(len(buf)) + ";\n")
        g.write("static const int _doc_data_uncompressed_size = " + str(decomp_size) + ";\n")
        g.write("static const unsigned char _doc_data_compressed[] = {\n")
        for i in range(len(buf)):
            g.write("\t" + str(buf[i]) + ",\n")
        g.write("};\n")

        g.write("#endif")

        g.close()

    make_doc_header("doc_data_compressed.gen.h", sorted(docs))

def gen_exporters(exporter_platforms):
    # Register exporters
    reg_exporters_inc = '#include "register_exporters.h"\n'
    reg_exporters = "void register_exporters() {\n"
    for e in exporter_platforms:
        reg_exporters += "\tregister_" + e + "_exporter();\n"
        reg_exporters_inc += '#include "platform/' + e + '/export/export.h"\n'
    reg_exporters += "}\n"

    # NOTE: It is safe to generate this file here, since this is still executed serially
    with open("register_exporters.gen.cpp", "w", encoding="utf-8") as f:
        f.write(reg_exporters_inc)
        f.write(reg_exporters)

if __name__ == "__main__":
    if len(sys.argv) >= 2:
        op = sys.argv[1]
        match op:
            case "docs":
                gen_docs()
            case "translation":
                pass
            case "fonts":
                pass
            case "exporters":
                gen_exporters(sys.argv[2:])