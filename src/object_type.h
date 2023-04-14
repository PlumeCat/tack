#pragma once

#include <vector>
#include <string>

// struct ObjectType {
//     struct node {
//         uint32_t hash;
//         std::string key;
//         Value value;
//     };
// };

#include <jlib/hash_map.h>
using ObjectType = hash_map<std::string, Value>;

