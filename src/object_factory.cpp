/**
 * Author: Shivadharshan S
 */
#include <object_factory.hpp>
#include <cstring>
#include <cstdlib>
#include <stdexcept>

void ObjectFactory::registerClass(const ClassInfo &cls)
{
    ClassInfo copy = cls;
    computeLayout(copy);
    classes[copy.name] = std::move(copy);
    class_offset_to_name[classes.size() - 1] = cls.name;
}

void ObjectFactory::computeLayout(ClassInfo &cls)
{
    size_t offset = 0;
    for (const auto &field : cls.fields)
    {
        cls.fieldOffsets[field.name] = offset;
        switch (field.type)
        {
        case FieldType::INT:
            offset += sizeof(int);
            break;
        case FieldType::OBJECT:       // other class objects
            offset += sizeof(void *); // pointer size
            break;
        case FieldType::FLOAT:
            offset += sizeof(float);
        // TODO: add more types
        default:
            throw std::runtime_error("Unknown field type in computeLayout");
        }
    }
    cls.objectSize = offset;
}

void *ObjectFactory::createObject(const std::string &className)
{
    auto it = classes.find(className);
    if (it == classes.end())
        throw std::runtime_error("Class not registered: " + className);

    const ClassInfo &cls = it->second;
    size_t totalSize = cls.objectSize + sizeof(void *); // simple header size for metadata pointer

    void *rawMemory = heapAllocate(totalSize);
    if (!rawMemory)
        throw std::bad_alloc();

    // Store pointer to ClassInfo as object header
    *static_cast<const ClassInfo **>(rawMemory) = &cls;

    // Zero initialize object fields (after header)
    void *objectData = static_cast<void *>(static_cast<char *>(rawMemory) + sizeof(void *));
    std::memset(objectData, 0, cls.objectSize);

    return objectData;
}

void ObjectFactory::destroyObject(void *object)
{
    if (!object)
        return;

    // Move pointer back to header start to free
    void *rawMemory = static_cast<void *>(static_cast<char *>(object) - sizeof(void *));
    heapFree(rawMemory);
}

const ClassInfo *ObjectFactory::getClassInfo(const std::string &className) const
{
    auto it = classes.find(className);
    if (it == classes.end())
        return nullptr;
    return &it->second;
}

void ObjectFactory::buildVTable(int classIndex)
{
    ClassInfo &cls = classes[class_offset_to_name[classIndex]];

    if (cls.superClassIndex >= 0)
    {
        ClassInfo &superCls = classes[class_offset_to_name[cls.superClassIndex]];
        if (superCls.vtable.empty())
        {
            buildVTable(cls.superClassIndex);
        }
        cls.vtable = superCls.vtable; // inherit superclass vtable
    }

    for (MethodInfo &method : cls.methods)
    {
        if (!method.isVirtual)
            continue;

        bool overridden = false;
        for (size_t i = 0; i < cls.vtable.size(); i++)
        {
            if (cls.vtable[i]->name == method.name)
            {
                cls.vtable[i] = &method; // override vtable slot
                overridden = true;
                break;
            }
        }

        // If not overriding, append to vtable
        if (!overridden)
        {
            cls.vtable.push_back(&method);
        }
    }
}

void ObjectFactory::buildAllVTables()
{
    for (size_t i = 0; i < classes.size(); i++)
    {
        if (classes[class_offset_to_name[i]].vtable.empty())
        {
            buildVTable(i);
        }
    }
}

void *ObjectFactory::heapAllocate(size_t size)
{
    return std::malloc(size);
}

void ObjectFactory::heapFree(void *ptr)
{
    std::free(ptr);
}
