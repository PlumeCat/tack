#pragma once


using Value = double;


#define types()\
    type(Null)\
    type(Boolean)\
    type(Number)\
    type(String)\
    type(Pointer)\
    type(Vec2) type(Vec3) type(Vec4) type(Mat4)\
    type(Array) type(Object) type(Function) type(Closure)

#define type(x) x,
enum class Type : uint8_t { types() };
#undef type
#define type(x) { Type::x, #x },
string to_string(Type type) {
    static hash_map<Type, string> type_to_string = { types() };
    return type_to_string[type];
}
#undef type


double nan_pack_int(uint32_t i) {
    // TODO: endianness
    auto top_16 = (uint64_t(0b0111111111111111)) << 48;
    auto _i = uint64_t(i) | top_16; // bottom bits not all 0
    auto d = *reinterpret_cast<double*>(&_i);
    return d;
}
uint32_t nan_unpack_int(double d) {
    auto i = *reinterpret_cast<uint64_t*>(&d);
    return i & 0xffffffff;
}
