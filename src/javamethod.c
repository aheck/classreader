/*
The MIT License (MIT) 
Copyright (c) 2009,2010,2016 Andreas Heck <aheck@gmx.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "javamethod.h"

#include <stdio.h>
#include <string.h>

/*
 * Method access and property bitmasks
 */
#define JAVAMETHOD_ACC_PUBLIC       0x0001
#define JAVAMETHOD_ACC_PRIVATE      0x0002
#define JAVAMETHOD_ACC_PROTECTED    0x0004
#define JAVAMETHOD_ACC_STATIC       0x0008
#define JAVAMETHOD_ACC_FINAL        0x0010
#define JAVAMETHOD_ACC_SYNCHRONIZED 0x0020
#define JAVAMETHOD_ACC_BRIDGE       0x0040
#define JAVAMETHOD_ACC_VARARGS      0x0080
#define JAVAMETHOD_ACC_NATIVE       0x0100
#define JAVAMETHOD_ACC_ABSTRACT     0x0400
#define JAVAMETHOD_ACC_STRICT       0x0800
#define JAVAMETHOD_ACC_SYNTHETIC    0x1000

JavaMethod* javamethod_new(guint16 access_flags, const gchar *name,
        const gchar *descriptor, const gchar *signature,
        const gchar **exceptions)
{
    JavaMethod *method = NULL;

    method = g_new(JavaMethod, 1);
    method->access_flags = access_flags;
    method->name = g_strdup(name);
    method->descriptor = g_strdup(descriptor);
    method->signature = g_strdup(signature);
    method->exceptions = NULL;
    method->code = NULL;
    method->codelen = 0;

    if (exceptions != NULL) {
        int len;
        for (len = 0; exceptions[len]; len++);

        method->exceptions = g_new(gchar*, len + 1);
        method->exceptions[len] = NULL; // NULL terminate array

        for (int i = 0; exceptions[i]; i++) {
            method->exceptions[i] = g_strdup(exceptions[i]);
        }
    }

    return method;
}

const gchar* javamethod_get_name(JavaMethod *method)
{
    return method->name;
}

const gchar* javamethod_get_descriptor(JavaMethod *method)
{
    return method->descriptor;
}

const gchar* javamethod_get_signature(JavaMethod *method)
{
    return method->signature;
}

gchar** javamethod_get_exceptions(JavaMethod *method)
{
    return method->exceptions;
}

gboolean javamethod_is_public(JavaMethod *method)
{
    return method->access_flags & JAVAMETHOD_ACC_PUBLIC ? TRUE : FALSE;
}

gboolean javamethod_is_protected(JavaMethod *method)
{
    return method->access_flags & JAVAMETHOD_ACC_PROTECTED ? TRUE : FALSE;
}

gboolean javamethod_is_private(JavaMethod *method)
{
    return method->access_flags & JAVAMETHOD_ACC_PRIVATE ? TRUE : FALSE;
}

gboolean javamethod_is_static(JavaMethod *method)
{
    return method->access_flags & JAVAMETHOD_ACC_STATIC ? TRUE : FALSE;
}

gboolean javamethod_is_final(JavaMethod *method)
{
    return method->access_flags & JAVAMETHOD_ACC_FINAL ? TRUE : FALSE;
}

gboolean javamethod_is_synchronized(JavaMethod *method)
{
    return method->access_flags & JAVAMETHOD_ACC_SYNCHRONIZED ? TRUE : FALSE;
}

gboolean javamethod_is_bridge_method(JavaMethod *method)
{
    return method->access_flags & JAVAMETHOD_ACC_BRIDGE ? TRUE : FALSE;
}

gboolean javamethod_has_varargs(JavaMethod *method)
{
    return method->access_flags & JAVAMETHOD_ACC_VARARGS ? TRUE : FALSE;
}

gboolean javamethod_is_abstract(JavaMethod *method)
{
    return method->access_flags & JAVAMETHOD_ACC_ABSTRACT ? TRUE : FALSE;
}

void javamethod_set_is_public(JavaMethod *method, gboolean value)
{
    if (value == TRUE)
        method->access_flags |= JAVAMETHOD_ACC_PUBLIC;
    else
        method->access_flags &= ~JAVAMETHOD_ACC_PUBLIC;
}

void javamethod_set_is_protected(JavaMethod *method, gboolean value)
{
    if (value == TRUE)
        method->access_flags |= JAVAMETHOD_ACC_PROTECTED;
    else
        method->access_flags &= ~JAVAMETHOD_ACC_PROTECTED;
}

void javamethod_set_is_private(JavaMethod *method, gboolean value)
{
    if (value == TRUE)
        method->access_flags |= JAVAMETHOD_ACC_PRIVATE;
    else
        method->access_flags &= ~JAVAMETHOD_ACC_PRIVATE;
}

void javamethod_set_is_abstract(JavaMethod *method, gboolean value)
{
    if (value == TRUE)
        method->access_flags |= JAVAMETHOD_ACC_ABSTRACT;
    else
        method->access_flags &= ~JAVAMETHOD_ACC_ABSTRACT;
}

void javamethod_set_is_static(JavaMethod *method, gboolean value)
{
    if (value == TRUE)
        method->access_flags |= JAVAMETHOD_ACC_STATIC;
    else
        method->access_flags &= ~JAVAMETHOD_ACC_STATIC;
}

void javamethod_set_is_final(JavaMethod *method, gboolean value)
{
    if (value == TRUE)
        method->access_flags |= JAVAMETHOD_ACC_FINAL;
    else
        method->access_flags &= ~JAVAMETHOD_ACC_FINAL;
}

void javamethod_set_is_synchronized(JavaMethod *method,
        gboolean value)
{
    if (value == TRUE)
        method->access_flags |= JAVAMETHOD_ACC_SYNCHRONIZED;
    else
        method->access_flags &= ~JAVAMETHOD_ACC_SYNCHRONIZED;
}

void javamethod_set_code(JavaMethod *method, guchar *code, guint32 codelen)
{
    if (code == NULL || codelen <= 0) return;

    method->code = g_malloc(codelen);
    memcpy(method->code, code, codelen);
    method->codelen = codelen;
}

void javamethod_free(JavaMethod *method)
{
    if (method != NULL) {
        g_free(method->name);
        g_free(method->descriptor);
        g_free(method->signature);
        g_free(method->code);
        g_strfreev(method->exceptions);

        g_free(method);
    }
}
