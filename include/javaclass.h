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
 * Implementation of a subset of the Java Class File format as
 * described in JSR 202
 */


#ifndef __JAVACLASS_H__
#define __JAVACLASS_H__

#include <glib.h>

#include "javafield.h"
#include "javamethod.h"

/*
 * GLib error handling
 */

#define JAVACLASS_GERROR g_quark_from_static_string("JAVACLASS_GERROR")

typedef enum
{
    JAVACLASS_ERROR_UNSUPPORTED_VERSION,
    JAVACLASS_ERROR_TAG_UNKNOWN
} JavaClassGError;

/*
 * Types used to represent a Java class file and its contents
 */

typedef union _cp_value
{
    gchar *str;
    gint32 i;
    gfloat f;
    gint64 l;
    gdouble d;
    guint16 index;
    guint16 indexpair[2];
} cp_value;

typedef struct _cp_info
{
    guchar tag;
    cp_value value; // we use a union here instead of a pointer to a
                    // specialized struct per tag like the spec does because
                    // on a 64 bit machine each pointer takes 8 bytes anyway
                    // and we would only waste memory if we stored a pointer
                    // to a <= 8 bytes struct and we also don't have to do more
                    // mallocs after allocating the initial cp_info array excpet
                    // for UTF-8 entries
} cp_info;

typedef struct _attribute_info
{
    guint16 attribute_name_index;
    guint32 attribute_length;
    guchar *info;
} attribute_info;

typedef struct _field_info
{
    guint16 access_flags;
    guint16 name_index;
    guint16 descriptor_index;
    guint16 attributes_count;
    attribute_info *attributes;
} field_info;

typedef struct _method_info
{
    guint16 access_flags;
    guint16 name_index;
    guint16 descriptor_index;
    guint16 attributes_count;
    attribute_info *attributes;
} method_info;

typedef struct _JavaClass
{
   guint32 magic_number;
   guint16 minor_version;
   guint16 major_version;
   guint16 constant_pool_count;
   cp_info *constant_pool;

   guint16 access_flags;
   guint16 this_class;
   guint16 super_class;
 
   guint16 interfaces_count;
   guint16 *interfaces;
 
   guint16 fields_count;
   field_info *fields;
 
   guint16 methods_count;
   method_info *methods;
 
   guint16 attributes_count;
   attribute_info *attributes;

   // convenience attributes for getters
   gchar *_package;
   gchar *_classname;
   gchar **_interfaces;
   JavaField **_fields;
   JavaMethod **_methods;
   gchar *_signature;
} JavaClass;

/*
 * Methods of the JavaClass structure
 */

/*
 * Create a new JavaClass object from an array of all the bytes of this class
 */
JavaClass* javaclass_new(guchar *classbytes, guint32 length, gboolean includecode, GError **error);

/*
 * Create a new JavaClass object from a filename
 */
JavaClass* javaclass_new_from_file(gchar *filename, gboolean includecode, GError **error);

/*
 * Get the unqualified name of this class
 */
const gchar* javaclass_get_name(JavaClass *c);

/*
 * Get the name of the package this class belongs to
 */
const gchar* javaclass_get_package(JavaClass *c);

/*
 * Get the fully qualified name of this class
 */
const gchar* javaclass_get_fq_name(JavaClass *c);

/*
 * Get the fully qualified name of the direct parent class
 */
const gchar* javaclass_get_fq_parent(JavaClass *c);

/*
 * Is this class public or can it only be accessed from its own package
 */
gboolean javaclass_is_public(JavaClass *c);

/*
 * Is this class final
 */
gboolean javaclass_is_final(JavaClass *c);

/*
 * Is this class an interface or a real class
 */
gboolean javaclass_is_interface(JavaClass *c);

/*
 * Is this class abstract or does it contain method implementations
 */
gboolean javaclass_is_abstract(JavaClass *c);

/*
 * Is this class an annotation
 */
gboolean javaclass_is_annotation(JavaClass *c);

/*
 * Is this class an enumeration
 */
gboolean javaclass_is_enum(JavaClass *c);

/*
 * Get the number of interfaces implemented by this class
 */
guint16 javaclass_get_interface_number(JavaClass *c);

/*
 * Get the fully-qualified names of all interfaces implemented by this class
 */
gchar** javaclass_get_interfaces(JavaClass *c);

/*
 * Get the number of fields of this class (doesn't include inherited fields)
 */
guint16 javaclass_get_field_number(JavaClass *c);

/*
 * Get the fields of this class (doesn't include inherited fields)
 */
JavaField** javaclass_get_fields(JavaClass *c);

/*
 * Get the number of methods of this class (doesn't include inherited methods)
 */
guint16 javaclass_get_method_number(JavaClass *c);

/*
 * Get the methods of this class (doesn't include inherited methods)
 */
JavaMethod** javaclass_get_methods(JavaClass *c);

/*
 * Get the major version number of a class file
 */
guint16 javaclass_get_major_version_number(JavaClass *c);

/*
 * Get the minor version number of a class file
 */
guint16 javaclass_get_minor_version_number(JavaClass *c);

/*
 * Get the string name of the Java version this class file was created with
 */
const gchar* javaclass_get_version_name(JavaClass *c);

/*
 * Get the signature of the class if it has any (otherwise returns NULL)
 */
const gchar* javaclass_get_signature(JavaClass *c);

/*
 * Extract the classname component from a fully qualified classname
 */
gchar* javaclass_extract_classname(const gchar *fqn);

/*
 * Extract the package component from a fully qualified classname
 */
gchar* javaclass_extract_package(const gchar *fqn);

/*
 * Free all the memory occupied by a JavaClass object
 */
void javaclass_free(JavaClass*);

#endif /* __JAVACLASS_H__ */
