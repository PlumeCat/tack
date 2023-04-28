#include "value.h"
#include "khash2.h"

// KHASH_MAP_INIT_STR(SV, Value) // expanded inline

ObjectType::ObjectType() {
    refcount = 0;
    marker = false;
    // hash = kh_init_SV();
}

//ObjectType::ObjectType(ObjectType&& from) {
//    hash = from.hash;
//    from.hash = nullptr;
//}
//ObjectType& ObjectType::operator=(ObjectType&& from) {
//    hash = from.hash;
//    from.hash = nullptr;
//    return *this;
//}
ObjectType::~ObjectType() {
    /*if (hash) {
        kh_destroy_SV(hash);
    }*/
}

uint32_t ObjectType::length() {
    return len;
}
void ObjectType::set(const std::string& key, Value val) {
    auto ret = 0;
    auto n = hash.put(key, &ret);

    len += (ret >= 1) ? 1 : 0;
    hash.value_at(n) = val;
}
Value ObjectType::get(const std::string& key, bool& found) {
    auto n = hash.get(key);
    if (n == hash.end()) {
        found = false;
        return value_null();
    }
    found = true;
    return hash.value_at(n);
}

ObjectType::iter ObjectType::begin() {
    auto i = hash.begin();
    while (i != end() && !hash.exist(i)) {
        i++;
    }
    return i;
}
ObjectType::iter ObjectType::next(ObjectType::iter i) {
    i++;
    while (i != end() && !hash.exist(i)) {
        i++;
    }
    return i;
}
const std::string& ObjectType::key(ObjectType::iter i) {
    return hash.key_at(i);
}
Value ObjectType::value(ObjectType::iter i) {
    return hash.value_at(i);
}
ObjectType::iter ObjectType::end() {
    return hash.end();
}
