#!/usr/bin/env python

import sys

# -*- coding: ibm850 -*-

template_typed = """
#ifdef TYPED_METHOD_BIND
template<class T $ifret ,class R$ $ifargs ,$ $arg, class P@$>
class MethodBind$argc$$ifret R$$ifconst C$ : public MethodBind {
public:

	$ifret R$ $ifnoret void$ (T::*method)($arg, P@$) $ifconst const$;
	virtual Variant::Type _gen_argument_type(int p_arg) const { return _get_argument_type(p_arg); }
#ifdef DEBUG_METHODS_ENABLED
	virtual GodotTypeInfo::Metadata get_argument_meta(int p_arg) const {
		$ifret if (p_arg==-1) return GetTypeInfo<R>::METADATA;$
		$arg if (p_arg==(@-1)) return GetTypeInfo<P@>::METADATA;
		$
		return GodotTypeInfo::METADATA_NONE;
	}
#endif
	Variant::Type _get_argument_type(int p_argument) const {
		$ifret if (p_argument==-1) return (Variant::Type)GetTypeInfo<R>::VARIANT_TYPE;$
		$arg if (p_argument==(@-1)) return (Variant::Type)GetTypeInfo<P@>::VARIANT_TYPE;
		$
		return Variant::NIL;
	}
	virtual PropertyInfo _gen_argument_type_info(int p_argument) const {
		$ifret if (p_argument==-1) return GetTypeInfo<R>::get_class_info();$
		$arg if (p_argument==(@-1)) return GetTypeInfo<P@>::get_class_info();
		$
		return PropertyInfo();
	}
	virtual String get_instance_class() const {
		return T::get_class_static();
	}

	virtual Variant call(Object* p_object,const Variant** p_args,int p_arg_count, Variant::CallError& r_error) {

		T *instance=Object::cast_to<T>(p_object);
		r_error.error=Variant::CallError::CALL_OK;
#ifdef DEBUG_METHODS_ENABLED

		ERR_FAIL_COND_V(!instance,Variant());
		if (p_arg_count>get_argument_count()) {
			r_error.error=Variant::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
			r_error.argument=get_argument_count();
			return Variant();

		}
		if (p_arg_count<(get_argument_count()-get_default_argument_count())) {

			r_error.error=Variant::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
			r_error.argument=get_argument_count()-get_default_argument_count();
			return Variant();
		}
		$arg CHECK_ARG(@);
		$
#endif
		$ifret Variant ret = $(instance->*method)($arg, _VC(@)$);
		$ifret return Variant(ret);$
		$ifnoret return Variant();$
	}

#ifdef PTRCALL_ENABLED
	virtual void ptrcall(Object*p_object,const void** p_args,void *r_ret) {

		T *instance=Object::cast_to<T>(p_object);
		$ifret PtrToArg<R>::encode( $ (instance->*method)($arg, PtrToArg<P@>::convert(p_args[@-1])$) $ifret ,r_ret)$ ;
	}
#endif
	MethodBind$argc$$ifret R$$ifconst C$ () {
#ifdef DEBUG_METHODS_ENABLED
		_set_const($ifconst true$$ifnoconst false$);
#endif
		_generate_argument_types($argc$);

		$ifret _set_returns(true); $
	};
};

template<class T $ifret ,class R$ $ifargs ,$ $arg, class P@$>
MethodBind* create_method_bind($ifret R$ $ifnoret void$ (T::*p_method)($arg, P@$) $ifconst const$ ) {

	MethodBind$argc$$ifret R$$ifconst C$<T $ifret ,R$ $ifargs ,$ $arg, P@$> * a = memnew( (MethodBind$argc$$ifret R$$ifconst C$<T $ifret ,R$ $ifargs ,$ $arg, P@$>) );
	a->method=p_method;
	return a;
}
#endif
"""

template = """
#ifndef TYPED_METHOD_BIND
$iftempl template<$ $ifret class R$ $ifretargs ,$ $arg, class P@$ $iftempl >$
class MethodBind$argc$$ifret R$$ifconst C$ : public MethodBind {

public:

	StringName type_name;
	$ifret R$ $ifnoret void$ (__UnexistingClass::*method)($arg, P@$) $ifconst const$;

	virtual Variant::Type _gen_argument_type(int p_arg) const { return _get_argument_type(p_arg); }
#ifdef DEBUG_METHODS_ENABLED
	virtual GodotTypeInfo::Metadata get_argument_meta(int p_arg) const {
		$ifret if (p_arg==-1) return GetTypeInfo<R>::METADATA;$
		$arg if (p_arg==(@-1)) return GetTypeInfo<P@>::METADATA;
		$
		return GodotTypeInfo::METADATA_NONE;
	}
#endif

	Variant::Type _get_argument_type(int p_argument) const {
		$ifret if (p_argument==-1) return (Variant::Type)GetTypeInfo<R>::VARIANT_TYPE;$
		$arg if (p_argument==(@-1)) return (Variant::Type)GetTypeInfo<P@>::VARIANT_TYPE;
		$
		return Variant::NIL;
	}

	virtual PropertyInfo _gen_argument_type_info(int p_argument) const {
		$ifret if (p_argument==-1) return GetTypeInfo<R>::get_class_info();$
		$arg if (p_argument==(@-1)) return GetTypeInfo<P@>::get_class_info();
		$
		return PropertyInfo();
	}

	virtual String get_instance_class() const {
		return type_name;
	}

	virtual Variant call(Object* p_object,const Variant** p_args,int p_arg_count, Variant::CallError& r_error) {

		__UnexistingClass *instance = (__UnexistingClass*)p_object;

		r_error.error=Variant::CallError::CALL_OK;
#ifdef DEBUG_METHODS_ENABLED

		ERR_FAIL_COND_V(!instance,Variant());
		if (p_arg_count>get_argument_count()) {
			r_error.error=Variant::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
			r_error.argument=get_argument_count();
			return Variant();
		}

		if (p_arg_count<(get_argument_count()-get_default_argument_count())) {

			r_error.error=Variant::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
			r_error.argument=get_argument_count()-get_default_argument_count();
			return Variant();
		}

		$arg CHECK_ARG(@);
		$
#endif
		$ifret Variant ret = $(instance->*method)($arg, _VC(@)$);
		$ifret return Variant(ret);$
		$ifnoret return Variant();$
	}
#ifdef PTRCALL_ENABLED
	virtual void ptrcall(Object*p_object,const void** p_args,void *r_ret) {
		__UnexistingClass *instance = (__UnexistingClass*)p_object;
		$ifret PtrToArg<R>::encode( $ (instance->*method)($arg, PtrToArg<P@>::convert(p_args[@-1])$) $ifret ,r_ret) $ ;
	}
#endif
	MethodBind$argc$$ifret R$$ifconst C$ () {
#ifdef DEBUG_METHODS_ENABLED
		_set_const($ifconst true$$ifnoconst false$);
#endif
		_generate_argument_types($argc$);
		$ifret _set_returns(true); $


	};
};

template<class T $ifret ,class R$ $ifargs ,$ $arg, class P@$>
MethodBind* create_method_bind($ifret R$ $ifnoret void$ (T::*p_method)($arg, P@$) $ifconst const$ ) {

	MethodBind$argc$$ifret R$$ifconst C$ $iftempl <$  $ifret R$ $ifretargs ,$ $arg, P@$ $iftempl >$ * a = memnew( (MethodBind$argc$$ifret R$$ifconst C$ $iftempl <$ $ifret R$ $ifretargs ,$ $arg, P@$ $iftempl >$) );
	union {

		$ifret R$ $ifnoret void$ (T::*sm)($arg, P@$) $ifconst const$;
		$ifret R$ $ifnoret void$ (__UnexistingClass::*dm)($arg, P@$) $ifconst const$;
	} u;
	u.sm=p_method;
	a->method=u.dm;
	a->type_name=T::get_class_static();
	return a;
}
#endif
"""


template_typed_free_func = """
#ifdef TYPED_METHOD_BIND
template<class T $ifret ,class R$ $ifargs ,$ $arg, class P@$>
class FunctionBind$argc$$ifret R$$ifconst C$ : public MethodBind {
public:

	$ifret R$ $ifnoret void$ (*method) ($ifconst const$ T *$ifargs , $$arg, P@$);
	virtual Variant::Type _gen_argument_type(int p_arg) const { return _get_argument_type(p_arg); }
	virtual GodotTypeInfo::Metadata get_argument_meta(int p_arg) const {
		$ifret if (p_arg==-1) return GetTypeInfo<R>::METADATA;$
		$arg if (p_arg==(@-1)) return GetTypeInfo<P@>::METADATA;
		$
		return GodotTypeInfo::METADATA_NONE;
	}
	Variant::Type _get_argument_type(int p_argument) const {
		$ifret if (p_argument==-1) return (Variant::Type)GetTypeInfo<R>::VARIANT_TYPE;$
		$arg if (p_argument==(@-1)) return (Variant::Type)GetTypeInfo<P@>::VARIANT_TYPE;
		$
		return Variant::NIL;
	}
	virtual PropertyInfo _gen_argument_type_info(int p_argument) const {
		$ifret if (p_argument==-1) return GetTypeInfo<R>::get_class_info();$
		$arg if (p_argument==(@-1)) return GetTypeInfo<P@>::get_class_info();
		$
		return PropertyInfo();
	}
	virtual String get_instance_class() const {
		return T::get_class_static();
	}

	virtual Variant call(Object* p_object,const Variant** p_args,int p_arg_count, Variant::CallError& r_error) {

		T *instance=Object::cast_to<T>(p_object);
		r_error.error=Variant::CallError::CALL_OK;
#ifdef DEBUG_METHODS_ENABLED

		ERR_FAIL_COND_V(!instance,Variant());
		if (p_arg_count>get_argument_count()) {
			r_error.error=Variant::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
			r_error.argument=get_argument_count();
			return Variant();

		}
		if (p_arg_count<(get_argument_count()-get_default_argument_count())) {

			r_error.error=Variant::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
			r_error.argument=get_argument_count()-get_default_argument_count();
			return Variant();
		}
		$arg CHECK_ARG(@);
		$
#endif
		$ifret Variant ret = $(method)(instance$ifargs , $$arg, _VC(@)$);
		$ifret return Variant(ret);$
		$ifnoret return Variant();$
	}

#ifdef PTRCALL_ENABLED
	virtual void ptrcall(Object*p_object,const void** p_args,void *r_ret) {

		T *instance=Object::cast_to<T>(p_object);
		$ifret PtrToArg<R>::encode( $ (method)(instance$ifargs , $$arg, PtrToArg<P@>::convert(p_args[@-1])$) $ifret ,r_ret)$ ;
	}
#endif
	FunctionBind$argc$$ifret R$$ifconst C$ () {
#ifdef DEBUG_METHODS_ENABLED
		_set_const($ifconst true$$ifnoconst false$);
#endif
		_generate_argument_types($argc$);

		$ifret _set_returns(true); $
	};
};

template<class T $ifret ,class R$ $ifargs ,$ $arg, class P@$>
MethodBind* create_method_bind($ifret R$ $ifnoret void$ (*p_method)($ifconst const$ T *$ifargs , $$arg, P@$) ) {

	FunctionBind$argc$$ifret R$$ifconst C$<T $ifret ,R$ $ifargs ,$ $arg, P@$> * a = memnew( (FunctionBind$argc$$ifret R$$ifconst C$<T $ifret ,R$ $ifargs ,$ $arg, P@$>) );
	a->method=p_method;
	return a;
}
#endif
"""


def make_version(template, nargs, argmax, const, ret):
    intext = template
    from_pos = 0
    outtext = ""

    while True:
        to_pos = intext.find("$", from_pos)
        if to_pos == -1:
            outtext += intext[from_pos:]
            break
        else:
            outtext += intext[from_pos:to_pos]
        end = intext.find("$", to_pos + 1)
        if end == -1:
            break  # ignore
        macro = intext[to_pos + 1 : end]
        cmd = ""
        data = ""

        if macro.find(" ") != -1:
            cmd = macro[0 : macro.find(" ")]
            data = macro[macro.find(" ") + 1 :]
        else:
            cmd = macro

        if cmd == "argc":
            outtext += str(nargs)
        if cmd == "ifret" and ret:
            outtext += data
        if cmd == "ifargs" and nargs:
            outtext += data
        if cmd == "ifretargs" and nargs and ret:
            outtext += data
        if cmd == "ifconst" and const:
            outtext += data
        elif cmd == "ifnoconst" and not const:
            outtext += data
        elif cmd == "ifnoret" and not ret:
            outtext += data
        elif cmd == "iftempl" and (nargs > 0 or ret):
            outtext += data
        elif cmd == "arg,":
            for i in range(1, nargs + 1):
                if i > 1:
                    outtext += ", "
                outtext += data.replace("@", str(i))
        elif cmd == "arg":
            for i in range(1, nargs + 1):
                outtext += data.replace("@", str(i))
        elif cmd == "noarg":
            for i in range(nargs + 1, argmax + 1):
                outtext += data.replace("@", str(i))

        from_pos = end + 1

    return outtext


def gen_bindings(target):
    versions = 13
    versions_ext = 6
    text = ""
    text_ext = ""
    text_free_func = "#ifndef METHOD_BIND_FREE_FUNC_H\n#define METHOD_BIND_FREE_FUNC_H\n"
    text_free_func += "\n//including this header file allows method binding to use free functions\n"
    text_free_func += (
        "//note that the free function must have a pointer to an instance of the class as its first parameter\n"
    )

    for i in range(0, versions + 1):
        t = ""
        t += make_version(template, i, versions, False, False)
        t += make_version(template_typed, i, versions, False, False)
        t += make_version(template, i, versions, False, True)
        t += make_version(template_typed, i, versions, False, True)
        t += make_version(template, i, versions, True, False)
        t += make_version(template_typed, i, versions, True, False)
        t += make_version(template, i, versions, True, True)
        t += make_version(template_typed, i, versions, True, True)
        if i >= versions_ext:
            text_ext += t
        else:
            text += t

        text_free_func += make_version(template_typed_free_func, i, versions, False, False)
        text_free_func += make_version(template_typed_free_func, i, versions, False, True)
        text_free_func += make_version(template_typed_free_func, i, versions, True, False)
        text_free_func += make_version(template_typed_free_func, i, versions, True, True)

    text_free_func += "#endif"

    with open(target[0], "w") as f:
        f.write(text)

    with open(target[1], "w") as f:
        f.write(text_ext)

    with open(target[2], "w") as f:
        f.write(text_free_func)


def make_certs_header(target):
    src = source[0]
    dst = target[0]
    f = open(src, "rb")
    g = open_utf8(dst, "w")
    buf = f.read()
    decomp_size = len(buf)
    import zlib

    buf = zlib.compress(buf)

    g.write("/* THIS FILE IS GENERATED DO NOT EDIT */\n")
    g.write("#ifndef _CERTS_RAW_H\n")
    g.write("#define _CERTS_RAW_H\n")

    # System certs path. Editor will use them if defined. (for package maintainers)
    path = env["system_certs_path"]
    g.write('#define _SYSTEM_CERTS_PATH "%s"\n' % str(path))
    if env["builtin_certs"]:
        # Defined here and not in env so changing it does not trigger a full rebuild.
        g.write("#define BUILTIN_CERTS_ENABLED\n")
        g.write("static const int _certs_compressed_size = " + str(len(buf)) + ";\n")
        g.write("static const int _certs_uncompressed_size = " + str(decomp_size) + ";\n")
        g.write("static const unsigned char _certs_compressed[] = {\n")
        for i in range(len(buf)):
            g.write("\t" + byte_to_str(buf[i]) + ",\n")
        g.write("};\n")
    g.write("#endif")

    g.close()
    f.close()


def make_authors_header(target):
    sections = ["Project Founders", "Lead Developer", "Project Manager", "Developers"]
    sections_id = ["AUTHORS_FOUNDERS", "AUTHORS_LEAD_DEVELOPERS", "AUTHORS_PROJECT_MANAGERS", "AUTHORS_DEVELOPERS"]

    src = source[0]
    dst = target[0]
    f = open_utf8(src, "r")
    g = open_utf8(dst, "w")

    g.write("/* THIS FILE IS GENERATED DO NOT EDIT */\n")
    g.write("#ifndef _EDITOR_AUTHORS_H\n")
    g.write("#define _EDITOR_AUTHORS_H\n")

    reading = False

    def close_section():
        g.write("\t0\n")
        g.write("};\n")

    for line in f:
        if reading:
            if line.startswith("    "):
                g.write('\t"' + escape_string(line.strip()) + '",\n')
                continue
        if line.startswith("## "):
            if reading:
                close_section()
                reading = False
            for section, section_id in zip(sections, sections_id):
                if line.strip().endswith(section):
                    current_section = escape_string(section_id)
                    reading = True
                    g.write("const char *const " + current_section + "[] = {\n")
                    break

    if reading:
        close_section()

    g.write("#endif\n")

    g.close()
    f.close()


def make_donors_header(target):
    sections = [
        "Platinum sponsors",
        "Gold sponsors",
        "Silver sponsors",
        "Bronze sponsors",
        "Mini sponsors",
        "Gold donors",
        "Silver donors",
        "Bronze donors",
    ]
    sections_id = [
        "DONORS_SPONSOR_PLATINUM",
        "DONORS_SPONSOR_GOLD",
        "DONORS_SPONSOR_SILVER",
        "DONORS_SPONSOR_BRONZE",
        "DONORS_SPONSOR_MINI",
        "DONORS_GOLD",
        "DONORS_SILVER",
        "DONORS_BRONZE",
    ]

    src = source[0]
    dst = target[0]
    f = open_utf8(src, "r")
    g = open_utf8(dst, "w")

    g.write("/* THIS FILE IS GENERATED DO NOT EDIT */\n")
    g.write("#ifndef _EDITOR_DONORS_H\n")
    g.write("#define _EDITOR_DONORS_H\n")

    reading = False

    def close_section():
        g.write("\t0\n")
        g.write("};\n")

    for line in f:
        if reading >= 0:
            if line.startswith("    "):
                g.write('\t"' + escape_string(line.strip()) + '",\n')
                continue
        if line.startswith("## "):
            if reading:
                close_section()
                reading = False
            for section, section_id in zip(sections, sections_id):
                if line.strip().endswith(section):
                    current_section = escape_string(section_id)
                    reading = True
                    g.write("const char *const " + current_section + "[] = {\n")
                    break

    if reading:
        close_section()

    g.write("#endif\n")

    g.close()
    f.close()


def make_license_header(target):
    src_copyright = source[0]
    src_license = source[1]
    dst = target[0]

    class LicenseReader:
        def __init__(self, license_file):
            self._license_file = license_file
            self.line_num = 0
            self.current = self.next_line()

        def next_line(self):
            line = self._license_file.readline()
            self.line_num += 1
            while line.startswith("#"):
                line = self._license_file.readline()
                self.line_num += 1
            self.current = line
            return line

        def next_tag(self):
            if not ":" in self.current:
                return ("", [])
            tag, line = self.current.split(":", 1)
            lines = [line.strip()]
            while self.next_line() and self.current.startswith(" "):
                lines.append(self.current.strip())
            return (tag, lines)

    from collections import OrderedDict

    projects = OrderedDict()
    license_list = []

    with open_utf8(src_copyright, "r") as copyright_file:
        reader = LicenseReader(copyright_file)
        part = {}
        while reader.current:
            tag, content = reader.next_tag()
            if tag in ("Files", "Copyright", "License"):
                part[tag] = content[:]
            elif tag == "Comment":
                # attach part to named project
                projects[content[0]] = projects.get(content[0], []) + [part]

            if not tag or not reader.current:
                # end of a paragraph start a new part
                if "License" in part and not "Files" in part:
                    # no Files tag in this one, so assume standalone license
                    license_list.append(part["License"])
                part = {}
                reader.next_line()

    data_list = []
    for project in itervalues(projects):
        for part in project:
            part["file_index"] = len(data_list)
            data_list += part["Files"]
            part["copyright_index"] = len(data_list)
            data_list += part["Copyright"]

    with open_utf8(dst, "w") as f:

        f.write("/* THIS FILE IS GENERATED DO NOT EDIT */\n")
        f.write("#ifndef _EDITOR_LICENSE_H\n")
        f.write("#define _EDITOR_LICENSE_H\n")
        f.write("const char *const GODOT_LICENSE_TEXT =")

        with open_utf8(src_license, "r") as license_file:
            for line in license_file:
                escaped_string = escape_string(line.strip())
                f.write('\n\t\t"' + escaped_string + '\\n"')
        f.write(";\n\n")

        f.write(
            "struct ComponentCopyrightPart {\n"
            "\tconst char *license;\n"
            "\tconst char *const *files;\n"
            "\tconst char *const *copyright_statements;\n"
            "\tint file_count;\n"
            "\tint copyright_count;\n"
            "};\n\n"
        )

        f.write(
            "struct ComponentCopyright {\n"
            "\tconst char *name;\n"
            "\tconst ComponentCopyrightPart *parts;\n"
            "\tint part_count;\n"
            "};\n\n"
        )

        f.write("const char *const COPYRIGHT_INFO_DATA[] = {\n")
        for line in data_list:
            f.write('\t"' + escape_string(line) + '",\n')
        f.write("};\n\n")

        f.write("const ComponentCopyrightPart COPYRIGHT_PROJECT_PARTS[] = {\n")
        part_index = 0
        part_indexes = {}
        for project_name, project in iteritems(projects):
            part_indexes[project_name] = part_index
            for part in project:
                f.write(
                    '\t{ "'
                    + escape_string(part["License"][0])
                    + '", '
                    + "&COPYRIGHT_INFO_DATA["
                    + str(part["file_index"])
                    + "], "
                    + "&COPYRIGHT_INFO_DATA["
                    + str(part["copyright_index"])
                    + "], "
                    + str(len(part["Files"]))
                    + ", "
                    + str(len(part["Copyright"]))
                    + " },\n"
                )
                part_index += 1
        f.write("};\n\n")

        f.write("const int COPYRIGHT_INFO_COUNT = " + str(len(projects)) + ";\n")

        f.write("const ComponentCopyright COPYRIGHT_INFO[] = {\n")
        for project_name, project in iteritems(projects):
            f.write(
                '\t{ "'
                + escape_string(project_name)
                + '", '
                + "&COPYRIGHT_PROJECT_PARTS["
                + str(part_indexes[project_name])
                + "], "
                + str(len(project))
                + " },\n"
            )
        f.write("};\n\n")

        f.write("const int LICENSE_COUNT = " + str(len(license_list)) + ";\n")

        f.write("const char *const LICENSE_NAMES[] = {\n")
        for l in license_list:
            f.write('\t"' + escape_string(l[0]) + '",\n')
        f.write("};\n\n")

        f.write("const char *const LICENSE_BODIES[] = {\n\n")
        for l in license_list:
            for line in l[1:]:
                if line == ".":
                    f.write('\t"\\n"\n')
                else:
                    f.write('\t"' + escape_string(line) + '\\n"\n')
            f.write('\t"",\n\n')
        f.write("};\n\n")

        f.write("#endif\n")

if __name__ == "__main__":
    method = sys.argv[1]
    match method:
    	case "bindings":
    		gen_bindings([
    			"method_bind.gen.inc",
    			"method_bind_ext.gen.inc",
    			"method_bind_free_func.gen.inc"])
    	case "info":
    		make_license_header("license.gen.h")
    		make_authors_header("authors.gen.h")
    		make_donors_header("donors.gen.h")
    	case "certs":
    		make_certs_header()
    	case _:
    		sys.stderr.write("Unknown method: %s" % method)
    		sys.exit(1)
