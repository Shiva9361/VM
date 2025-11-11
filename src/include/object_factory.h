#ifndef VM_OBJECT_FACTORY_H
#define VM_OBJECT_FACTORY_H

#include "common.h"
#include "string.h"
#include "vector.h"
#include "hash_map.h"

// Forward declarations
typedef struct ClassInfo_t ClassInfo_t;
typedef struct Object_t Object_t; // Represents a generic object on the heap

typedef enum FieldType_t
{
    FIELD_TYPE_INT = 1,
    FIELD_TYPE_OBJECT = 2,
    FIELD_TYPE_FLOAT = 3,
    FIELD_TYPE_CHAR = 4,
} FieldType_t;

typedef struct FieldInfo_t
{
    string_t name;
    FieldType_t type;
} FieldInfo_t;

typedef struct MethodInfo_t
{
    string_t name;
    uint32_t bytecodeOffset;
    bool isVirtual; // Default to true
} MethodInfo_t;

// Function pointer for virtual table entries
typedef void (*MethodPtr)(void *);

struct ClassInfo_t
{
    string_t name;
    int32_t superClassIndex; // -1 if none
    vector_t fields;         // vector_t<FieldInfo_t>
    vector_t methods;        // vector_t<MethodInfo_t>
    vector_t vtable;         // vector_t<MethodPtr>
    hash_map_t fieldOffsets; // hash_map_t<string_t, size_t>
    size_t objectSize;
};

typedef struct ObjectFactory_t
{
    hash_map_t class_offset_to_name; // hash_map_t<int, string_t>
    hash_map_t classes;              // hash_map_t<string_t, ClassInfo_t>
    vector_t heap;                   // vector_t<void*> - simple heap for now
    size_t next_heap_addr;           // Simple incrementing address for heap allocation
} ObjectFactory_t;

// ObjectFactory_t function declarations
void object_factory_init(ObjectFactory_t *factory);
void object_factory_destroy(ObjectFactory_t *factory);
bool object_factory_register_class(ObjectFactory_t *factory, const ClassInfo_t *cls);
Object_t *object_factory_create_object(ObjectFactory_t *factory, const string_t *className);
void object_factory_destroy_object(ObjectFactory_t *factory, Object_t *object);
const ClassInfo_t *object_factory_get_class_info(const ObjectFactory_t *factory, const string_t *className);
void object_factory_build_vtable(ObjectFactory_t *factory, const string_t classIndex);
void object_factory_build_all_vtables(ObjectFactory_t *factory);

// Helper functions for ClassInfo_t
void class_info_init(ClassInfo_t *class_info);
void class_info_destroy(ClassInfo_t *class_info);

#endif // VM_OBJECT_FACTORY_H
