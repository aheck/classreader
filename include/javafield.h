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

/*
 * Representation of a single field of a Java class
 */

#ifndef __JAVAFIELD_H__
#define __JAVAFIELD_H__

#include <glib.h>

typedef struct _JavaField
{
    guint16 access_flags;
    gchar *name;
    gchar *descriptor;
    gchar *signature;
} JavaField;

/*
 * Create a new JavaField instance
 */
JavaField* javafield_new(guint16 access_flags, const gchar *name,
        const gchar *descriptor, const gchar *signature);

/*
 * Get the name of the field of a Java class
 */
const gchar* javafield_get_name(JavaField *field);

/*
 * Get the type descriptor of the field of a Java class
 */
const gchar* javafield_get_descriptor(JavaField *field);

/*
 * Get the signature of the field of a Java class
 */
const gchar* javafield_get_signature(JavaField *field);

/*
 * Is this field declared public?
 */
gboolean javafield_is_public(JavaField *field);

/*
 * Is this field declared protected?
 */
gboolean javafield_is_protected(JavaField *field);

/*
 * Is this field declared private?
 */
gboolean javafield_is_private(JavaField *field);

/*
 * Is this field declared static?
 */
gboolean javafield_is_static(JavaField *field);

/*
 * Is this field declared final?
 */
gboolean javafield_is_final(JavaField *field);

/*
 * Is this field an element of an enum?
 */
gboolean javafield_is_enum(JavaField *field);

/*
 * Convert a JavaField to code that can be used to declare it in Java
 *
 * The string returned by this method will look something like
 *
 * "public String myfieldname"
 */
void javafield_to_string(JavaField *field, GString *buffer);

/*
 * Set the public flag of the field
 */
void javafield_set_is_public(JavaField *field, gboolean value);

/*
 * Set the protected flag of the field
 */
void javafield_set_is_protected(JavaField *field, gboolean value);

/*
 * Set the private flag of the field
 */
void javafield_set_is_private(JavaField *field, gboolean value);

/*
 * Set the static flag of the field
 */
void javafield_set_is_static(JavaField *field, gboolean value);

/*
 * Set the final flag of the field
 */
void javafield_set_is_final(JavaField *field, gboolean value);

/*
 * Set the enum flag of the field
 */
void javafield_set_is_enum(JavaField *field, gboolean value);

/*
 * Free a JavaField instance
 */
void javafield_free(JavaField *field);

#endif /* __JAVAFIELD_H__ */
