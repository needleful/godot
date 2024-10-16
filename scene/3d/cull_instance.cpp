/*************************************************************************/
/*  cull_instance.cpp                                                    */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "cull_instance.h"

void CullInstance::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_include_in_bound", "enabled"), &CullInstance::set_include_in_bound);
	ClassDB::bind_method(D_METHOD("get_include_in_bound"), &CullInstance::get_include_in_bound);

	ClassDB::bind_method(D_METHOD("set_allow_merging", "enabled"), &CullInstance::set_allow_merging);
	ClassDB::bind_method(D_METHOD("get_allow_merging"), &CullInstance::get_allow_merging);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "include_in_bound"), "set_include_in_bound", "get_include_in_bound");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow_merging"), "set_allow_merging", "get_allow_merging");
}

CullInstance::CullInstance() {
	_include_in_bound = true;
	_allow_merging = true;
}
