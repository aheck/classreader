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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "javaclass.h"

#define MAX_MAJOR_VERSION 50

/*
 * Tags used to classify entries in the constant pool
 */
#define TAG_UTF8                1
#define TAG_INTEGER             3
#define TAG_FLOAT               4
#define TAG_LONG                5
#define TAG_DOUBLE              6
#define TAG_CLASS               7
#define TAG_STRING              8
#define TAG_FIELDREF            9
#define TAG_METHODREF          10
#define TAG_INTERFACEMETHODREF 11
#define TAG_NAMEANDTYPE        12

/*
 * Class access and property bitmasks
 */
#define JAVACLASS_ACC_PUBLIC     0x0001
#define JAVACLASS_ACC_FINAL      0x0010
#define JAVACLASS_ACC_SUPER      0x0020
#define JAVACLASS_ACC_INTERFACE  0x0200
#define JAVACLASS_ACC_ABSTRACT   0x0400
#define JAVACLASS_ACC_SYNTHETIC  0x1000
#define JAVACLASS_ACC_ANNOTATION 0x2000
#define JAVACLASS_ACC_ENUM       0x4000

/*
 * Macros for inplace conversion from Big Endian
 * (used by the class file format) to host byte order
 */
#define GUINT16_CONV(variable) variable = GUINT16_FROM_BE(variable);
#define GUINT32_CONV(variable) variable = GUINT32_FROM_BE(variable);
#define GINT32_CONV(variable) variable = GINT32_FROM_BE(variable);
#define GINT64_CONV(variable) variable = GINT64_FROM_BE(variable);

#define INVALID_INDEX 65535

/*
 * Error codes
 */
#define JAVACLASS_ERROR_READING_FILE 1

/*
 * Return a string from the constant pool
 */
static gchar* string_from_cp(JavaClass *c, guint16 i)
{
    g_assert(c->constant_pool[i].tag == TAG_UTF8);
    return c->constant_pool[i].value.str;
}

/*
 * Return the name of a class from a constant pool index to the classref
 */
static gchar* classname_from_cp(JavaClass *c, guint16 i)
{
    g_assert(c->constant_pool[i].tag == TAG_CLASS);
    g_assert(c->constant_pool[c->constant_pool[i].value.index].tag == TAG_UTF8);
    return c->constant_pool[c->constant_pool[i].value.index].value.str;
}

/*
 * Copy some bytes from a offset into a array and increment the offset counter
 */
static void copy_bytes(void *dest, const void *src, guint32 *offset, guint32 num)
{
    char *srcoff = (char*) src + *offset;

    memcpy(dest, (void*) srcoff, num);
    *offset += num;
}

/*
 * Skip some bytes in case we don't need a part of the class file
 */
static void skip_bytes(guint32 *offset, guint32 num)
{
    *offset += num;
}

/*
 * Convert an classname in the internal format to the external format
 *
 * Internal format uses '/' as delimiter while external format uses '.'
 * So for example 'java/lang/Object' becomes 'java.lang.Object'
 */
static void classname_to_external_format(gchar *str)
{
    while (*str != 0) {
        if (*str == '/') *str = '.';
        str++;
    }
}

/*
 * Find out if a given attribute name belongs to those we are interested in
 */
static gboolean is_known_attribute(gchar *attribute)
{
    gchar *known_attributes[] = {"Exceptions", "SourceFile", "Signature",
        "Code", NULL};

    for (int i = 0; known_attributes[i]; i++) {
        if (g_strcmp0(attribute, known_attributes[i]) == 0) return TRUE;
    }

    return FALSE;
}

/*
 * Read the constant pool of a Java class file
 */
static void read_constant_pool(JavaClass *c, guchar *classbytes,
        guint32 *offset, GError **error)
{
    guint16 slen = 0;
    cp_info *cur = NULL;

    for (int i = 0; i < c->constant_pool_count; i++) {
        cur = &c->constant_pool[i];
        copy_bytes(&cur->tag, classbytes, offset, 1);
        switch (cur->tag) {
            case TAG_UTF8:
                copy_bytes(&slen, classbytes, offset, 2);
                GUINT16_CONV(slen);

                // FIXME: Convert the string to real UTF-8
                cur->value.str = (gchar*) g_malloc(slen + 1);
                copy_bytes(cur->value.str, classbytes, offset, slen);
                cur->value.str[slen] = '\0';
                break;
            case TAG_INTEGER:
                copy_bytes(&cur->value.i, classbytes, offset, 4);
                GINT32_CONV(cur->value.i);
                break;
            case TAG_FLOAT:
                copy_bytes(&cur->value.f, classbytes, offset, 4);
                GINT32_CONV(cur->value.f);
                break;
            case TAG_LONG:
                copy_bytes(&cur->value.l, classbytes, offset, 8);
                GINT64_CONV(cur->value.l);

                // LONGs occupy two slots
                i++;
                cur = &c->constant_pool[i];
                cur->tag = TAG_LONG;

                break;
            case TAG_DOUBLE:
                copy_bytes(&cur->value.d, classbytes, offset, 8);
                GINT64_CONV(cur->value.d);

                // DOUBLEs occupy two slots
                i++;
                cur = &c->constant_pool[i];
                cur->tag = TAG_DOUBLE;

                break;
            case TAG_CLASS:
                // same as TAG_STRING
            case TAG_STRING:
                copy_bytes(&cur->value.index, classbytes, offset, 2);
                GUINT16_CONV(cur->value.index);
                cur->value.index--;
                break;
            case TAG_FIELDREF:
                // same as METHODREF, INTERFACEMETHODREF, and
                // NAMEANDTYPE
            case TAG_METHODREF:
                // same as FIELDREF, INTERFACEMETHODREF, and
                // NAMEANDTYPE
            case TAG_INTERFACEMETHODREF:
                // same as FIELDREF, METHODREF, NAMEANDTYPE
            case TAG_NAMEANDTYPE:
                copy_bytes(&cur->value.indexpair[0], classbytes, offset, 2);
                GUINT16_CONV(cur->value.indexpair[0]);
                cur->value.indexpair[0]--;

                copy_bytes(&cur->value.indexpair[1], classbytes, offset, 2);
                GUINT16_CONV(cur->value.indexpair[1]);
                cur->value.indexpair[1]--;
                break;
            default:
                g_set_error(error,
                        JAVACLASS_GERROR,
                        JAVACLASS_ERROR_TAG_UNKNOWN,
                        "Error parsing class file: Unknown constant pool tag %d\n", cur->tag);
                return;
        }
    }
}

/*
 * Read an attribute section of a Java class file
 */
static void read_attributes(JavaClass *c, attribute_info *attributes,
        guchar *classbytes, guint32 *offset, guint16 attributes_count,
        GError **error)
{
    attribute_info *cur = NULL;

    for (int i = 0; i < attributes_count; i++) {
        cur = &attributes[i];

        copy_bytes(&cur->attribute_name_index, classbytes, offset, 2);
        GUINT16_CONV(cur->attribute_name_index);
        cur->attribute_name_index--;

        copy_bytes(&cur->attribute_length, classbytes, offset, 4);
        GUINT32_CONV(cur->attribute_length);

        if (is_known_attribute(string_from_cp(c, cur->attribute_name_index))) {
            cur->info = g_new(guchar, cur->attribute_length);
            copy_bytes(cur->info, classbytes, offset, cur->attribute_length);
        } else {
            cur->info = NULL;
            skip_bytes(offset, cur->attribute_length);
        }
    }
}

/*
 * Read the fields section of a Java class file
 */
static void read_fields(JavaClass *c, guchar *classbytes, guint32 *offset,
        GError **error)
{
    field_info *cur = NULL;
    GError *suberror = NULL;

    for (int i = 0; i < c->fields_count; i++) {
        cur = &c->fields[i];

        copy_bytes(&cur->access_flags, classbytes, offset, 2);
        GUINT16_CONV(cur->access_flags);

        copy_bytes(&cur->name_index, classbytes, offset, 2);
        GUINT16_CONV(cur->name_index);
        cur->name_index--;

        copy_bytes(&cur->descriptor_index, classbytes, offset, 2);
        GUINT16_CONV(cur->descriptor_index);
        cur->descriptor_index--;

        copy_bytes(&cur->attributes_count, classbytes, offset, 2);
        GUINT16_CONV(cur->attributes_count);

        cur->attributes = g_new(attribute_info, cur->attributes_count);
        read_attributes(c, cur->attributes, classbytes, offset,
                cur->attributes_count, &suberror);

        if (suberror != NULL) {
            g_propagate_error(error, suberror);
            return;
        }
    }
}

/*
 * Read the method section of a Java class file
 */
static void read_methods(JavaClass *c, guchar *classbytes,
        guint32 *offset, GError **error)
{
    method_info *cur = NULL;
    GError *suberror = NULL;

    for (int i = 0; i < c->methods_count; i++) {
        cur = &c->methods[i];

        copy_bytes(&cur->access_flags, classbytes, offset, 2);
        GUINT16_CONV(cur->access_flags);

        copy_bytes(&cur->name_index, classbytes, offset, 2);
        GUINT16_CONV(cur->name_index);
        cur->name_index--;

        copy_bytes(&cur->descriptor_index, classbytes, offset, 2);
        GUINT16_CONV(cur->descriptor_index);
        cur->descriptor_index--;

        copy_bytes(&cur->attributes_count, classbytes, offset, 2);
        GUINT16_CONV(cur->attributes_count);

        cur->attributes = g_new(attribute_info, cur->attributes_count);
        read_attributes(c, cur->attributes, classbytes, offset,
                cur->attributes_count, &suberror);

        if (suberror != NULL) {
            g_propagate_error(error, suberror);
            return;
        }
    }
}

/*
 * Extract the exceptions from the method attributes
 */
static gchar** extract_exceptions(JavaClass *c, attribute_info* attributes,
        guint16 count)
{
    gchar **exceptions = NULL;

    if (count <= 0) return NULL;

    // search for the exception attribute
    for (int i = 0; i < count; i++) {
        gchar *name = string_from_cp(c, attributes[i].attribute_name_index);
        if (g_strcmp0("Exceptions", name) == 0) {
            guchar *info = attributes[i].info;
            guint16 num_exceptions = 0;
            guint16 curindex = 0;
            guint32 offset = 0;

            copy_bytes(&num_exceptions, info, &offset, 2);
            GUINT16_CONV(num_exceptions);

            if (num_exceptions <= 0) return NULL;

            exceptions = g_new(gchar*, num_exceptions + 1);
            exceptions[num_exceptions] = NULL; // NULL terminate array

            for (int i = 0; i < num_exceptions; i++) {
                copy_bytes(&curindex, info, &offset, 2);
                GUINT16_CONV(curindex);
                curindex--;

                classname_to_external_format(classname_from_cp(c, curindex));
                exceptions[i] = classname_from_cp(c, curindex);
            }

            return exceptions;
        }
    }

    return NULL;
}

JavaClass* javaclass_new(guchar *classbytes, guint32 length, gboolean includecode, GError **error)
{
    JavaClass *c = NULL;
    guint32 offset = 0;
    GError *suberror = NULL;
    c = g_new(JavaClass, 1);

    // initialize all pointers in the JavaClass struct with NULL so that we
    // can tell which don't point to allocated memory in case of an error
    c->constant_pool = NULL;
    c->interfaces    = NULL;
    c->fields        = NULL;
    c->methods       = NULL;
    c->attributes    = NULL;
    c->_package      = NULL;
    c->_classname    = NULL;
    c->_interfaces   = NULL;
    c->_fields       = NULL;
    c->_methods      = NULL;
    c->_signature    = NULL;

    g_assert(sizeof(gfloat) == 4);
    g_assert(sizeof(gdouble) == 8);

    // read the magic number
    copy_bytes(&c->magic_number, classbytes, &offset, 4);
    GUINT32_CONV(c->magic_number);

    // check the magic number
    if (c->magic_number != 0xCAFEBABE) {
        g_set_error(error,
                JAVACLASS_GERROR,
                JAVACLASS_ERROR_TAG_UNKNOWN,
                "Error parsing class file: File is not a valid CLASS file!\n");
        javaclass_free(c);
        return NULL;
    }

    // read the minor and major class format version numbers
    copy_bytes(&c->minor_version, classbytes, &offset, 2);
    GUINT16_CONV(c->minor_version);

    copy_bytes(&c->major_version, classbytes, &offset, 2);
    GUINT16_CONV(c->major_version);

    // check if we support this version of the class file format
    if (c->major_version > MAX_MAJOR_VERSION) {
        g_set_error(error,
                JAVACLASS_GERROR,
                JAVACLASS_ERROR_UNSUPPORTED_VERSION,
                "Error parsing class file: The version of the class file format used by this class file is not supported, yet!\n");
        javaclass_free(c);
        return NULL;
    }

    // read the constant table count
    copy_bytes(&c->constant_pool_count, classbytes, &offset, 2);
    GUINT16_CONV(c->constant_pool_count);
    // we count from 0 not from 1 like the Java class file format
    c->constant_pool_count--;

    // allocate space for the constant pool
    c->constant_pool = g_new(cp_info,  c->constant_pool_count);

    read_constant_pool(c, classbytes, &offset, &suberror);

    if (suberror != NULL) {
        g_propagate_error(error, suberror);
        javaclass_free(c);
        return NULL;
    }

    // read the access flags
    copy_bytes(&c->access_flags, classbytes, &offset, 2);
    GUINT16_CONV(c->access_flags);

    // read this class index
    copy_bytes(&c->this_class, classbytes, &offset, 2);
    GUINT16_CONV(c->this_class);
    c->this_class--;
    classname_to_external_format(classname_from_cp(c, c->this_class));

    // read superclass index
    copy_bytes(&c->super_class, classbytes, &offset, 2);
    GUINT16_CONV(c->super_class);
    c->super_class--;
    // Do we have a super class? java.lang.object doesn't have one!
    if (c->super_class != INVALID_INDEX) {
        classname_to_external_format(classname_from_cp(c, c->super_class));
    }

    // read the interfaces count
    copy_bytes(&c->interfaces_count, classbytes, &offset, 2);
    GUINT16_CONV(c->interfaces_count);

    // read the interfaces list
    if (c->interfaces_count > 0) {
        guint16 len = c->interfaces_count * 2;
        c->interfaces = g_new(guint16, len);
        copy_bytes(c->interfaces, classbytes, &offset, len);

        for (int i = 0; i < c->interfaces_count; i++) {
            GUINT16_CONV(c->interfaces[i]);
            c->interfaces[i]--;
        }
    }

    // read the fields count
    copy_bytes(&c->fields_count, classbytes, &offset, 2);
    GUINT16_CONV(c->fields_count);

    // read the fields list
    if (c->fields_count > 0) {
        c->fields = g_new(field_info, c->fields_count);
        read_fields(c, classbytes, &offset, &suberror);

        if (suberror != NULL) {
            g_propagate_error(error, suberror);
            javaclass_free(c);
            return NULL;
        }
    }

    // read the methods count
    copy_bytes(&c->methods_count, classbytes, &offset, 2);
    GUINT16_CONV(c->methods_count);

    // read the methods list
    if (c->methods_count > 0) {
        c->methods = g_new(method_info, c->methods_count);
        read_methods(c, classbytes, &offset, &suberror);

        if (suberror != NULL) {
            g_propagate_error(error, suberror);
            javaclass_free(c);
            return NULL;
        }
    }

    // read the attributes count
    copy_bytes(&c->attributes_count, classbytes, &offset, 2);
    GUINT16_CONV(c->attributes_count);

    // read the attributes list of the class
    if (c->attributes_count > 0) {
        c->attributes = g_new(attribute_info, c->attributes_count);
        read_attributes(c, c->attributes, classbytes, &offset,
                c->attributes_count, &suberror);

        if (suberror != NULL) {
            g_propagate_error(error, suberror);
            javaclass_free(c);
            return NULL;
        }
    }

    // did we read to the end?
    g_assert(offset == length);

    /*
     * Fill additional data structures that are returned by some of the getters
     * and aren't derived from the CLASS file format but are there to allow
     * more convenient access to information exposed by the getters
     */

    c->_package = javaclass_extract_package(classname_from_cp(c, c->this_class));
    c->_classname = javaclass_extract_classname(classname_from_cp(c, c->this_class));

    if (c->interfaces_count > 0) {
        c->_interfaces = g_new(gchar*, c->interfaces_count + 1);
        c->_interfaces[c->interfaces_count] = NULL; // NULL terminate the array

        for (int i = 0; i < c->interfaces_count; i++) {
            classname_to_external_format(classname_from_cp(c, c->interfaces[i]));
            c->_interfaces[i] = classname_from_cp(c, c->interfaces[i]);
        }
    }

    if (c->fields_count > 0) {
        c->_fields = g_new(JavaField*, c->fields_count + 1);
        c->_fields[c->fields_count] = NULL; // NULL terminate the array

        for (int i = 0; i < c->fields_count; i++) {
            guint16 access_flags = c->fields[i].access_flags;
            gchar *name       = string_from_cp(c, c->fields[i].name_index);
            gchar *descriptor = string_from_cp(c, c->fields[i].descriptor_index);
            gchar *signature  = NULL;

            // extract the "Signature" attribute if any
            for (int j = 0; j < c->fields[i].attributes_count; j++) {
                guint16 name_index = 
                    c->fields[i].attributes[j].attribute_name_index;

                if (g_strcmp0("Signature", string_from_cp(c, name_index)) == 0) {
                    guint16 sig = 0;
                    memcpy(&sig, c->fields[i].attributes[j].info, 2);
                    GUINT16_CONV(sig);
                    sig--;

                    signature = string_from_cp(c, sig);
                }
            }

            c->_fields[i] = javafield_new(access_flags, name,
                    descriptor, signature);
        }
    }

    if (c->methods_count > 0) {
        c->_methods = g_new(JavaMethod*, c->methods_count + 1);
        c->_methods[c->methods_count] = NULL; // NULL terminate array

        for (int i = 0; i < c->methods_count; i++) {
            guint16 access_flags = c->methods[i].access_flags;
            gchar *name = string_from_cp(c, c->methods[i].name_index);
            gchar *descriptor = string_from_cp(c, c->methods[i].descriptor_index);
            gchar *signature = NULL;
            gchar **exceptions = NULL;

            exceptions = extract_exceptions(c, c->methods[i].attributes,
                    c->methods[i].attributes_count);
            guint32 codelen = 0;
            guchar *code = NULL;

            // extract the "Signature" attribute if any
            for (int j = 0; j < c->methods[i].attributes_count; j++) {
                guint16 name_index = 
                    c->methods[i].attributes[j].attribute_name_index;

                if (g_strcmp0("Signature", string_from_cp(c, name_index)) == 0) {
                    guint16 sig = 0;
                    memcpy(&sig, c->methods[i].attributes[j].info, 2);
                    GUINT16_CONV(sig);
                    sig--;

                    signature = string_from_cp(c, sig);
                } else if (includecode && g_strcmp0("Code", string_from_cp(c, name_index)) == 0) {
                    // use pointer arithmetic to get the codelen and the
                    // bytecode array from the "Code" attribute_info structure
                    codelen = *((guint32*) (c->methods[i].attributes[j].info + 4));
                    GUINT32_CONV(codelen);
                    code = c->methods[i].attributes[j].info + 8;
                }
            }

            c->_methods[i] = javamethod_new(access_flags, (const gchar*) name,
                    (const gchar*) descriptor, (const gchar*) signature,
                    (const gchar**) exceptions);
            if (includecode) javamethod_set_code(c->_methods[i], code, codelen);

            g_free(exceptions);
        }
    }

    // search the classes attributes
    for (int i = 0; i < c->attributes_count; i++)
    {
        const guint16 name_index = c->attributes[i].attribute_name_index;
        if (g_strcmp0("Signature", string_from_cp(c, name_index)) == 0) {
            guint16 sig = 0;
            memcpy(&sig, c->attributes[i].info, 2);
            GUINT16_CONV(sig);
            sig--;

            c->_signature = string_from_cp(c, sig);
        }
    }

    return c;
}

JavaClass* javaclass_new_from_file(gchar *filename, gboolean includecode, GError **error)
{
    struct stat buffer;
    guchar *classbytes = NULL;
    JavaClass *retval = NULL;
    int status;

    FILE *fp = fopen(filename, "r");

    // get the size of the file
    status = fstat(fileno(fp), &buffer);
    size_t len = buffer.st_size;

    // allocate buffer
    classbytes = g_new(guchar, len);

    size_t nbytes = fread(classbytes, 1, len, fp);
    fclose(fp);

    if (nbytes != len) {
        *error = g_error_new(0, JAVACLASS_ERROR_READING_FILE,
                "ERROR: Read error! Requested %zd bytes but got %zd!\n",
                len, nbytes);
    } else {
        retval = javaclass_new(classbytes, len, includecode, error);
    }

    g_free(classbytes);

    if (*error != NULL) return NULL;

    return retval;
}

const gchar* javaclass_get_name(JavaClass *c)
{
    return c->_classname;
}

const gchar* javaclass_get_package(JavaClass *c)
{
    return c->_package;
}

const gchar* javaclass_get_fq_name(JavaClass *c)
{
    return classname_from_cp(c, c->this_class);
}

const gchar* javaclass_get_fq_parent(JavaClass *c)
{
    if (c->super_class == INVALID_INDEX) return NULL;

    return classname_from_cp(c, c->super_class);
}

guint16 javaclass_get_interface_number(JavaClass *c)
{
    return c->interfaces_count;
}

gchar** javaclass_get_interfaces(JavaClass *c)
{
    return c->_interfaces;
}

guint16 javaclass_get_field_number(JavaClass *c)
{
    return c->fields_count;
}

JavaField** javaclass_get_fields(JavaClass *c)
{
    return c->_fields;
}

guint16 javaclass_get_method_number(JavaClass *c)
{
    return c->methods_count;
}

JavaMethod** javaclass_get_methods(JavaClass *c)
{
    return c->_methods;
}

guint16 javaclass_get_major_version_number(JavaClass *c)
{
    return c->major_version;
}

guint16 javaclass_get_minor_version_number(JavaClass *c)
{
    return c->minor_version;
}

const gchar* javaclass_get_version_name(JavaClass *c)
{
    switch(c->major_version) {
        case 50:
            return "J2SE 6.0";
        case 49:
            return "J2SE 5.0";
        case 48:
            return "JDK 1.4";
        case 47:
            return "JDK 1.3";
        case 46:
            return "JDK 1.2";
        case 45:
            return "JDK 1.1";
        default:
            return "UNKNOWN JAVA VERSION";
    }
}

gboolean javaclass_is_public(JavaClass *c)
{
    return c->access_flags & JAVACLASS_ACC_PUBLIC ? TRUE : FALSE;
}

gboolean javaclass_is_final(JavaClass *c)
{
    return c->access_flags & JAVACLASS_ACC_FINAL ? TRUE : FALSE;
}

gboolean javaclass_is_interface(JavaClass *c)
{
    return c->access_flags & JAVACLASS_ACC_INTERFACE ? TRUE : FALSE;
}

gboolean javaclass_is_abstract(JavaClass *c)
{
    return c->access_flags & JAVACLASS_ACC_ABSTRACT ? TRUE : FALSE;
}

gboolean javaclass_is_annotation(JavaClass *c)
{
    return c->access_flags & JAVACLASS_ACC_ANNOTATION ? TRUE : FALSE;
}

gboolean javaclass_is_enum(JavaClass *c)
{
    return c->access_flags & JAVACLASS_ACC_ENUM ? TRUE : FALSE;
}

const gchar* javaclass_get_signature(JavaClass *c)
{
    return c->_signature;
}

gchar* javaclass_extract_classname(const gchar *fqn)
{
    if (fqn == NULL) return NULL;

    gchar *pos = g_strrstr(fqn, ".");

    if (pos == NULL) return g_strdup(fqn);
    if (pos[1] == '\0') return NULL;

    return g_strdup(&pos[1]);
}

gchar* javaclass_extract_package(const gchar *fqn)
{
    if (fqn == NULL) return NULL;

    gchar *pos = g_strrstr(fqn, ".");

    if (pos == NULL) return NULL;

    return g_strndup(fqn, pos - fqn);
}

void javaclass_free(JavaClass *c)
{
    if (c != NULL) {
        if (c->constant_pool != NULL) {
            for (int i = 0; i < c->constant_pool_count; i++) {
                if (c->constant_pool[i].tag == TAG_UTF8)
                    g_free(c->constant_pool[i].value.str);
            }
        }

        g_free(c->constant_pool);
        g_free(c->interfaces);

        if (c->fields != NULL) {
            for (int i = 0; i < c->fields_count; i++) {
                for (int j = 0; j < c->fields[i].attributes_count; j++) {
                    g_free(c->fields[i].attributes[j].info);
                }

                g_free(c->fields[i].attributes);
            }
        }

        g_free(c->fields);

        if (c->methods != NULL) {
            for (int i = 0; i < c->methods_count; i++) {
                for (int j = 0; j < c->methods[i].attributes_count; j++)
                {
                    g_free(c->methods[i].attributes[j].info);
                }

                g_free(c->methods[i].attributes);
            }
        }

        g_free(c->methods);

        if (c->attributes != NULL) {
            for (int i = 0; i < c->attributes_count; i++) {
                g_free(c->attributes[i].info);
            }
        }

        if (c->_fields != NULL) {
            for (int i = 0; c->_fields[i]; i++) {
                javafield_free(c->_fields[i]);
            }

            g_free(c->_fields);
        }

        if (c->_methods != NULL) {
            for (int i = 0; c->_methods[i]; i++) {
                javamethod_free(c->_methods[i]);
            }

            g_free(c->_methods);
        }

        g_free(c->attributes);
        g_free(c->_package);
        g_free(c->_classname);
        g_free(c->_interfaces);
        g_free(c);
    }
}
