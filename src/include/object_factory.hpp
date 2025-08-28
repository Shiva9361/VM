#ifndef VM_OBJECT_FACTORY_HPP
#define VM_OBJECT_FACTORY_HPP

#include <string>
#include <vector>
#include <unordered_map>

enum class FieldType : uint8_t
{
    INT = 1,
    OBJECT = 2,
    // TODO: add more types :)
};

struct FieldInfo
{
    std::string name;
    FieldType type;
};

struct MethodInfo
{
    std::string name;
    uint32_t bytecodeOffset;
};

struct ClassInfo
{
    std::string name;
    int32_t superClassIndex; // -1 if none
    std::vector<FieldInfo> fields;
    std::vector<MethodInfo> methods;
    size_t objectSize; // TODO: compute this
};

class ObjectFactory
{
public:
    void registerClass(const ClassInfo &cls);
    void *createObject(const std::string &className);
    void destroyObject(void *object);
    const ClassInfo *getClassInfo(const std::string &className) const;

private:
    std::unordered_map<std::string, ClassInfo> classes;

    void computeLayout(ClassInfo &cls);
    void *heapAllocate(size_t size); // TODO: use a better allocator
    void heapFree(void *ptr);
};

#endif // VM_OBJECT_FACTORY_HPP