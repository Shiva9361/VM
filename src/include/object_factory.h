/**
 * Author: Shivadharshan S
 */
#ifndef VM_OBJECT_FACTORY_H
#define VM_OBJECT_FACTORY_H

#include <stdint.h>
#include <stdlib.h> // For malloc, free
#include <string.h> // For strcmp, strcpy

#define MAX_NAME_LEN 32 // Max length for class, field, method names
#define MAX_FIELDS_PER_CLASS 16
#define MAX_METHODS_PER_CLASS 16
#define MAX_CLASSES_REGISTERED 32 // Max number of classes ObjectFactory can handle

typedef enum {
    INT = 1,
    OBJECT = 2,
    FLOAT = 3,
    CHAR = 4,
} FieldType;

typedef struct {
    char name[MAX_NAME_LEN];
    FieldType type;
} FieldInfo;

typedef struct {
    char name[MAX_NAME_LEN];
    uint32_t bytecodeOffset;
    uint8_t isVirtual; // boolean
} MethodInfo;

// Forward declaration for ClassInfo to allow self-referencing in vtable
typedef struct ClassInfo ClassInfo;

struct ClassInfo {
    char name[MAX_NAME_LEN];
    int32_t superClassIndex; // -1 if none
    FieldInfo fields[MAX_FIELDS_PER_CLASS];
    uint32_t field_count;
    MethodInfo methods[MAX_METHODS_PER_CLASS];
    uint32_t method_count;
    MethodInfo* vtable[MAX_METHODS_PER_CLASS]; // pointers to methods for virtual dispatch
    uint32_t vtable_size;
    
    // For field offsets, we'll use a simple array lookup or linear search
    // For now, let's assume field names are unique and we can find their offset
    // by iterating through fields.
    uint32_t fieldOffsets[MAX_FIELDS_PER_CLASS]; // Stores actual byte offsets
    
    size_t objectSize; // Size of the object's data part (excluding metadata pointer)
};

typedef struct {
    ClassInfo classes[MAX_CLASSES_REGISTERED];
    uint32_t class_count;
    // A simple mapping from class name string to index in the classes array
    // For C, we might use a linear search or a simple hash table if performance is critical.
    // For now, linear search by name.
} ObjectFactory_State;

// Function prototypes for ObjectFactory operations
void ObjectFactory_init(ObjectFactory_State* factory);
void ObjectFactory_registerClass(ObjectFactory_State* factory, const ClassInfo* cls_template);
void* ObjectFactory_createObject(ObjectFactory_State* factory, const char* className);
void ObjectFactory_destroyObject(void* object);
const ClassInfo* ObjectFactory_getClassInfo(const ObjectFactory_State* factory, const char* className);
void ObjectFactory_buildVTable(ObjectFactory_State* factory, uint32_t classIndex);
void ObjectFactory_buildAllVTables(ObjectFactory_State* factory);

// Helper functions (private to implementation, but declared here for now)
void ObjectFactory_computeLayout(ClassInfo* cls);
void* ObjectFactory_heapAllocate(size_t size);
void ObjectFactory_heapFree(void* ptr);

#endif // VM_OBJECT_FACTORY_H
