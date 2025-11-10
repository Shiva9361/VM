/**
 * Author: Shivadharshan S
 */
#include "include/object_factory.h"

// System standard library includes
#include <stdio.h> // System's stdio.h for fprintf, stderr
#include <stdlib.h> // System's stdlib.h for malloc, free, exit
#include <string.h> // System's string.h for strcmp, strcpy, memcpy, memset

// Include user-space syscall headers from OS project
#include <unistd.h> // For write syscall

// Helper function to find a class by name
static int find_class_index(const ObjectFactory_State* factory, const char* className) {
    for (uint32_t i = 0; i < factory->class_count; ++i) {
        if (strcmp(factory->classes[i].name, className) == 0) {
            return i;
        }
    }
    return -1; // Not found
}

void ObjectFactory_init(ObjectFactory_State* factory) {
    factory->class_count = 0;
    // Initialize class names to empty strings
    for (uint32_t i = 0; i < MAX_CLASSES_REGISTERED; ++i) {
        factory->classes[i].name[0] = '\0';
    }
}

void ObjectFactory_registerClass(ObjectFactory_State* factory, const ClassInfo* cls_template) {
    if (factory->class_count >= MAX_CLASSES_REGISTERED) {
        write(2, "Error: Max classes registered in ObjectFactory.\n", strlen("Error: Max classes registered in ObjectFactory.\n"));
        return;
    }
    
    // Copy the class info
    ClassInfo* new_cls = &factory->classes[factory->class_count];
    memcpy(new_cls, cls_template, sizeof(ClassInfo));
    
    ObjectFactory_computeLayout(new_cls);
    factory->class_count++;
}

void ObjectFactory_computeLayout(ClassInfo* cls) {
    size_t offset = 0;
    for (uint32_t i = 0; i < cls->field_count; ++i) {
        cls->fieldOffsets[i] = offset;
        switch (cls->fields[i].type) {
            case INT:
                offset += sizeof(int);
                break;
            case OBJECT:
                offset += sizeof(void*);
                break;
            case FLOAT:
                offset += sizeof(float);
                break;
            case CHAR:
                offset += sizeof(char);
                break;
            default:
                write(2, "Error: Unknown field type in computeLayout.\n", strlen("Error: Unknown field type in computeLayout.\n"));
                exit(1);
        }
    }
    cls->objectSize = offset;
}

void* ObjectFactory_createObject(ObjectFactory_State* factory, const char* className) {
    int class_idx = find_class_index(factory, className);
    if (class_idx == -1) {
        write(2, "Error: Class not registered.\n", strlen("Error: Class not registered.\n"));
        return NULL;
    }

    const ClassInfo* cls = &factory->classes[class_idx];
    size_t totalSize = cls->objectSize + sizeof(void*); // simple header size for metadata pointer

    void* rawMemory = ObjectFactory_heapAllocate(totalSize);
    if (!rawMemory) {
        write(2, "Error: Failed to allocate memory for object.\n", strlen("Error: Failed to allocate memory for object.\n"));
        return NULL;
    }

    // Store pointer to ClassInfo as object header
    *(const ClassInfo**)rawMemory = cls;

    // Zero initialize object fields (after header)
    void* objectData = (void*)((char*)rawMemory + sizeof(void*));
    memset(objectData, 0, cls->objectSize);

    return objectData;
}

void ObjectFactory_destroyObject(void* object) {
    if (!object) return;

    // Move pointer back to header start to free
    void* rawMemory = (void*)((char*)object - sizeof(void*));
    ObjectFactory_heapFree(rawMemory);
}

const ClassInfo* ObjectFactory_getClassInfo(const ObjectFactory_State* factory, const char* className) {
    int class_idx = find_class_index(factory, className);
    if (class_idx == -1) {
        return NULL;
    }
    return &factory->classes[class_idx];
}

void ObjectFactory_buildVTable(ObjectFactory_State* factory, uint32_t classIndex) {
    ClassInfo* cls = &factory->classes[classIndex];

    if (cls->superClassIndex >= 0) {
        ClassInfo* superCls = &factory->classes[cls->superClassIndex];
        if (superCls->vtable_size == 0) { // If superclass vtable not built yet
            ObjectFactory_buildVTable(factory, cls->superClassIndex);
        }
        // Inherit superclass vtable
        memcpy(cls->vtable, superCls->vtable, superCls->vtable_size * sizeof(MethodInfo*));
        cls->vtable_size = superCls->vtable_size;
    }

    for (uint32_t i = 0; i < cls->method_count; ++i) {
        MethodInfo* method = &cls->methods[i];
        if (!method->isVirtual) continue;

        uint8_t overridden = 0;
        for (uint32_t j = 0; j < cls->vtable_size; ++j) {
            if (strcmp(cls->vtable[j]->name, method->name) == 0) {
                cls->vtable[j] = method; // override vtable slot
                overridden = 1;
                break;
            }
        }

        // If not overriding, append to vtable
        if (!overridden) {
            if (cls->vtable_size >= MAX_METHODS_PER_CLASS) {
                write(2, "Error: VTable overflow.\n", strlen("Error: VTable overflow.\n"));
                exit(1);
            }
            cls->vtable[cls->vtable_size++] = method;
        }
    }
}

void ObjectFactory_buildAllVTables(ObjectFactory_State* factory) {
    for (uint32_t i = 0; i < factory->class_count; ++i) {
        if (factory->classes[i].vtable_size == 0) {
            ObjectFactory_buildVTable(factory, i);
        }
    }
}

void* ObjectFactory_heapAllocate(size_t size) {
    return malloc(size);
}

void ObjectFactory_heapFree(void* ptr) {
    free(ptr);
}
