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

#include "javafield.h"

/*
 * Field access and property bitmasks
 */
#define JAVAFIELD_ACC_PUBLIC    0x0001
#define JAVAFIELD_ACC_PRIVATE   0x0002
#define JAVAFIELD_ACC_PROTECTED 0x0004
#define JAVAFIELD_ACC_STATIC    0x0008
#define JAVAFIELD_ACC_FINAL     0x0010
#define JAVAFIELD_ACC_VOLATILE  0x0040
#define JAVAFIELD_ACC_TRANSIENT 0x0080
#define JAVAFIELD_ACC_SYNTHETIC 0x1000
#define JAVAFIELD_ACC_ENUM      0x4000

JavaField* javafield_new(guint16 access_flags, const gchar *name,
        const gchar *descriptor, const gchar *signature)
{
    JavaField *field = NULL;

    field = g_new(JavaField, 1);
    field->access_flags = access_flags;
    field->name = g_strdup(name);
    field->descriptor = g_strdup(descriptor);
    field->signature = g_strdup(signature);

    return field;
}

const gchar* javafield_get_name(JavaField *field)
{
    return field->name;
}

const gchar* javafield_get_descriptor(JavaField *field)
{
    return field->descriptor;
}

const gchar* javafield_get_signature(JavaField *field)
{
    return field->signature;
}

gboolean javafield_is_public(JavaField *field)
{
    return field->access_flags & JAVAFIELD_ACC_PUBLIC ? TRUE : FALSE;
}

gboolean javafield_is_protected(JavaField *field)
{
    return field->access_flags & JAVAFIELD_ACC_PROTECTED ? TRUE : FALSE;
}

gboolean javafield_is_private(JavaField *field)
{
    return field->access_flags & JAVAFIELD_ACC_PRIVATE ? TRUE : FALSE;
}

gboolean javafield_is_static(JavaField *field)
{
    return field->access_flags & JAVAFIELD_ACC_STATIC ? TRUE : FALSE;
}

gboolean javafield_is_final(JavaField *field)
{
    return field->access_flags & JAVAFIELD_ACC_FINAL ? TRUE : FALSE;
}

gboolean javafield_is_enum(JavaField *field)
{
    return field->access_flags & JAVAFIELD_ACC_ENUM ? TRUE : FALSE;
}

void javafield_set_is_public(JavaField *field, gboolean value)
{
    if (value == TRUE)
        field->access_flags |= JAVAFIELD_ACC_PUBLIC;
    else
        field->access_flags &= ~JAVAFIELD_ACC_PUBLIC;
}

void javafield_set_is_protected(JavaField *field, gboolean value)
{
    if (value == TRUE)
        field->access_flags |= JAVAFIELD_ACC_PROTECTED;
    else
        field->access_flags &= ~JAVAFIELD_ACC_PROTECTED;
}

void javafield_set_is_private(JavaField *field, gboolean value)
{
    if (value == TRUE)
        field->access_flags |= JAVAFIELD_ACC_PRIVATE;
    else
        field->access_flags &= JAVAFIELD_ACC_PRIVATE;
}

void javafield_set_is_static(JavaField *field, gboolean value)
{
    if (value == TRUE)
        field->access_flags |= JAVAFIELD_ACC_STATIC;
    else
        field->access_flags &= ~JAVAFIELD_ACC_STATIC;
}

void javafield_set_is_final(JavaField *field, gboolean value)
{
    if (value == TRUE)
        field->access_flags |= JAVAFIELD_ACC_FINAL;
    else
        field->access_flags &= ~JAVAFIELD_ACC_FINAL;
}

void javafield_set_is_enum(JavaField *field, gboolean value)
{
    if (value == TRUE)
        field->access_flags |= JAVAFIELD_ACC_ENUM;
    else
        field->access_flags &= ~JAVAFIELD_ACC_ENUM;
}

void javafield_free(JavaField *field)
{
    if (field != NULL) {
        g_free(field->name);
        g_free(field->descriptor);
        g_free(field->signature);

        g_free(field);
    }
}
