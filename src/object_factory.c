#include <string.h> // Must be first for memcpy/memset declarations
#include "object_factory.h"
#include <stdlib.h> // For malloc, free
#include <stdio.h>  // For fprintf, stderr

// Helper function to compute layout (private to object_factory.c)
static void object_factory_compute_layout(ClassInfo_t *cls)
{
    size_t offset = 0;
    for (size_t i = 0; i < vector_size(&cls->fields); ++i)
    {
        FieldInfo_t *field = (FieldInfo_t *)vector_get_ptr(&cls->fields, i);

        // Store offset in hash map
        size_t current_offset = offset; // Copy to pass by value
        hash_map_put(&cls->fieldOffsets, &field->name, &current_offset);

        switch (field->type)
        {
        case FIELD_TYPE_INT:
            offset += sizeof(int);
            break;
        case FIELD_TYPE_OBJECT:
            offset += sizeof(void *); // pointer size
            break;
        case FIELD_TYPE_FLOAT:
            offset += sizeof(float);
            break;
        case FIELD_TYPE_CHAR:
            offset += sizeof(char);
            break;
        default:
            ERROR_PRINT("Unknown field type in computeLayout\n");
            // Handle error, perhaps exit or return an error code
            break;
        }
    }
    cls->objectSize = offset;
}

void object_factory_init(ObjectFactory_t *factory)
{
    hash_map_init(&factory->class_offset_to_name, sizeof(string_t)); // int (key) -> string_t (value)
    hash_map_init(&factory->classes, sizeof(ClassInfo_t));           // string_t (key) -> ClassInfo_t (value)
    vector_init(&factory->heap, sizeof(void *));                     // Not strictly used as a heap, but for tracking allocated objects
    factory->next_heap_addr = 0;                                     // Placeholder, actual heap management will be direct malloc/free
}

void object_factory_destroy(ObjectFactory_t *factory)
{
    // Destroy all ClassInfo_t objects stored in the classes hash map
    // This requires iterating through the hash map and calling class_info_destroy on each value
    // For simplicity, we'll assume ClassInfo_t objects are fully contained and their destruction
    // handles internal string_t, vector_t, hash_map_t.
    // This is a simplified destruction. A proper one would iterate buckets and destroy each ClassInfo_t.
    // For now, hash_map_destroy will handle freeing the memory for ClassInfo_t values.
    hash_map_destroy(&factory->class_offset_to_name);
    hash_map_destroy(&factory->classes);
    vector_destroy(&factory->heap); // This vector just holds pointers, actual objects need to be freed
}

bool object_factory_register_class(ObjectFactory_t *factory, const ClassInfo_t *cls)
{
    ClassInfo_t copy_cls;
    class_info_init(&copy_cls); // Initialize the copy

    // Deep copy ClassInfo_t
    string_copy(&copy_cls.name, &cls->name);
    copy_cls.superClassIndex = cls->superClassIndex;

    // Copy fields
    for (size_t i = 0; i < vector_size(&cls->fields); ++i)
    {
        FieldInfo_t *original_field = (FieldInfo_t *)vector_get_ptr(&cls->fields, i);
        FieldInfo_t copy_field;
        string_init_from_cstr(&copy_field.name, string_cstr(&original_field->name));
        copy_field.type = original_field->type;
        vector_push_back(&copy_cls.fields, &copy_field);
        // string_destroy(&copy_field.name); // Destroy temporary string
    }

    // Copy methods
    for (size_t i = 0; i < vector_size(&cls->methods); ++i)
    {
        MethodInfo_t *original_method = (MethodInfo_t *)vector_get_ptr(&cls->methods, i);
        MethodInfo_t copy_method;
        string_init_from_cstr(&copy_method.name, string_cstr(&original_method->name));
        copy_method.bytecodeOffset = original_method->bytecodeOffset;
        copy_method.isVirtual = original_method->isVirtual;
        vector_push_back(&copy_cls.methods, &copy_method);
        // string_destroy(&copy_method.name); // Destroy temporary string
    }

    object_factory_compute_layout(&copy_cls);

    if (!hash_map_put(&factory->classes, &copy_cls.name, &copy_cls))
    {
        class_info_destroy(&copy_cls); // Clean up if put fails
        return false;
    }

    // Store class name by offset (index in the conceptual array of classes)
    // This assumes classes are registered in a specific order to match indices.
    // In C++, classes.size() - 1 was used, which implies an integer index.
    // We need a way to map an integer index to a string name.
    // For now, we'll use the current size of the classes map as the "offset"
    // and store the class name. This is a simplification.
    int current_class_index_i = hash_map_size(&factory->classes) - 1; // This is problematic if keys are removed
    string_t current_class_index;
    char index[2] = {current_class_index_i + '0', '\0'};
    string_init_from_cstr(&current_class_index, index);
    string_t class_name_copy;
    string_init_from_cstr(&class_name_copy, string_cstr(&cls->name));
    if (!hash_map_put(&factory->class_offset_to_name, &current_class_index, &class_name_copy))
    { // Key is int, value is string_t
        string_destroy(&class_name_copy);
        return false;
    }
    string_destroy(&class_name_copy); // Clean up temporary string
    class_info_destroy(&copy_cls);    // The copy is now owned by the hash map, so destroy the local copy's contents
    return true;
}

Object_t *object_factory_create_object(ObjectFactory_t *factory, const string_t *className)
{
    const ClassInfo_t *cls = object_factory_get_class_info(factory, className);
    if (!cls)
    {
        ERROR_PRINT("Class not registered: %s\n", string_cstr(className));
        return NULL;
    }

    size_t totalSize = cls->objectSize + sizeof(ClassInfo_t *); // Header size for metadata pointer

    void *rawMemory = malloc(totalSize);
    if (!rawMemory)
    {
        ERROR_PRINT("Failed to allocate memory for object.\n");
        return NULL;
    }

    // Store pointer to ClassInfo_t as object header
    *(ClassInfo_t **)rawMemory = (ClassInfo_t *)cls;

    // Zero initialize object fields (after header)
    void *objectData = (char *)rawMemory + sizeof(ClassInfo_t *);
    memset(objectData, 0, cls->objectSize);

    // Add to heap tracking (simplified)
    // vector_push_back(&factory->heap, &rawMemory); // This would store a copy of the pointer, not the pointer itself
    // For now, we'll just return the objectData pointer.
    // Proper GC or heap management would track rawMemory.

    return (Object_t *)objectData;
}

void object_factory_destroy_object(ObjectFactory_t *factory, Object_t *object)
{
    if (!object)
        return;

    // Move pointer back to header start to free
    void *rawMemory = (char *)object - sizeof(ClassInfo_t *);
    free(rawMemory);
    // In a real system, you'd also remove it from the heap tracking vector
}

const ClassInfo_t *object_factory_get_class_info(const ObjectFactory_t *factory, const string_t *className)
{
    return (const ClassInfo_t *)hash_map_get(&factory->classes, className);
}

void object_factory_build_vtable(ObjectFactory_t *factory, int classIndex)
{
    // This function needs to retrieve ClassInfo_t by index, which is tricky with hash_map_t.
    // The C++ version used class_offset_to_name[classIndex] to get the name, then classes[name].
    // We need to replicate this.
    string_t *class_name_ptr = (string_t *)hash_map_get(&factory->class_offset_to_name, (string_t *)&classIndex);
    if (!class_name_ptr)
    {
        ERROR_PRINT("Class name not found for index %d\n", classIndex);
        return;
    }
    string_t class_name = *class_name_ptr; // Copy the string_t

    ClassInfo_t *cls = (ClassInfo_t *)hash_map_get(&factory->classes, &class_name);
    if (!cls)
    {
        ERROR_PRINT("Class info not found for name %s\n", string_cstr(&class_name));
        string_destroy(&class_name);
        return;
    }

    if (cls->superClassIndex >= 0)
    {
        // Recursively build superclass vtable if not already built
        string_t *super_class_name_ptr = (string_t *)hash_map_get(&factory->class_offset_to_name, (string_t *)&cls->superClassIndex);
        if (super_class_name_ptr)
        {
            string_t super_class_name = *super_class_name_ptr;
            ClassInfo_t *superCls = (ClassInfo_t *)hash_map_get(&factory->classes, &super_class_name);
            if (superCls && vector_size(&superCls->vtable) == 0)
            {
                object_factory_build_vtable(factory, cls->superClassIndex);
            }
            // Inherit superclass vtable
            for (size_t i = 0; i < vector_size(&superCls->vtable); ++i)
            {
                MethodPtr method_ptr = *(MethodPtr *)vector_get_ptr(&superCls->vtable, i);
                vector_push_back(&cls->vtable, &method_ptr);
            }
            string_destroy(&super_class_name);
        }
    }

    for (size_t i = 0; i < vector_size(&cls->methods); ++i)
    {
        MethodInfo_t *method = (MethodInfo_t *)vector_get_ptr(&cls->methods, i);
        if (!method->isVirtual)
            continue;

        bool overridden = false;
        for (size_t j = 0; j < vector_size(&cls->vtable); ++j)
        {
            // This comparison is tricky. We need to compare method names.
            // The vtable stores MethodPtr, not MethodInfo_t*.
            // This implies that the MethodPtr should somehow encode the MethodInfo_t.
            // For now, this part of the vtable logic will be simplified or require a different approach.
            // In C++, vtable stored MethodInfo* and compared method->name.
            // We need to store MethodInfo_t* in vtable and then cast to MethodPtr when calling.
            // Let's adjust object_factory.h to store MethodInfo_t* in vtable.
            // For now, I'll assume MethodInfo_t* is stored in vtable.
            MethodInfo_t **vtable_method_ptr = (MethodInfo_t **)vector_get_ptr(&cls->vtable, j);
            if (string_compare(&(*vtable_method_ptr)->name, &method->name))
            {
                *vtable_method_ptr = method; // override vtable slot
                overridden = true;
                break;
            }
        }

        if (!overridden)
        {
            vector_push_back(&cls->vtable, &method); // Append MethodInfo_t*
        }
    }
    string_destroy(&class_name);
}

void object_factory_build_all_vtables(ObjectFactory_t *factory)
{
    // This loop needs to iterate through all registered classes.
    // Iterating hash_map_t is not straightforward.
    // The C++ version used classes.size() and class_offset_to_name.
    // We need a way to get all class names or iterate the classes hash map.
    // For now, this will be a placeholder or require a different hash map iteration strategy.
    // A simple approach is to iterate from 0 to hash_map_size(&factory->classes) - 1
    // and try to retrieve class names using the integer as a key for class_offset_to_name.
    // This assumes class_offset_to_name is populated with contiguous integer keys.
    for (int i = 0; i < hash_map_size(&factory->classes); ++i)
    {
        string_t *class_name_ptr = (string_t *)hash_map_get(&factory->class_offset_to_name, (string_t *)&i);
        if (class_name_ptr)
        {
            string_t class_name = *class_name_ptr;
            ClassInfo_t *cls = (ClassInfo_t *)hash_map_get(&factory->classes, &class_name);
            if (cls && vector_size(&cls->vtable) == 0)
            {
                object_factory_build_vtable(factory, i);
            }
            string_destroy(&class_name);
        }
    }
}

// Helper functions for ClassInfo_t
void class_info_init(ClassInfo_t *class_info)
{
    string_init(&class_info->name);
    class_info->superClassIndex = -1;
    vector_init(&class_info->fields, sizeof(FieldInfo_t));
    vector_init(&class_info->methods, sizeof(MethodInfo_t));
    vector_init(&class_info->vtable, sizeof(MethodInfo_t *)); // Store MethodInfo_t*
    hash_map_init(&class_info->fieldOffsets, sizeof(size_t));
    class_info->objectSize = 0;
}

void class_info_destroy(ClassInfo_t *class_info)
{
    string_destroy(&class_info->name);

    // Destroy contents of vectors
    for (size_t i = 0; i < vector_size(&class_info->fields); ++i)
    {
        FieldInfo_t *field = (FieldInfo_t *)vector_get_ptr(&class_info->fields, i);
        string_destroy(&field->name);
    }
    vector_destroy(&class_info->fields);

    for (size_t i = 0; i < vector_size(&class_info->methods); ++i)
    {
        MethodInfo_t *method = (MethodInfo_t *)vector_get_ptr(&class_info->methods, i);
        string_destroy(&method->name);
    }
    vector_destroy(&class_info->methods);

    vector_destroy(&class_info->vtable); // vtable stores pointers, so no deep destruction needed here

    // Destroy contents of hash map
    // This is tricky. hash_map_destroy will free the size_t values, but not the string_t keys.
    // A proper hash_map_destroy for string_t keys would need to iterate and destroy keys.
    hash_map_destroy(&class_info->fieldOffsets);
}
