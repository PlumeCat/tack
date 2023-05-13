#include "interpreter.h"

// TODO: proper debug logging / diagnostics / monitoring system
#define debug(...)
#define dump(...)

void Heap::gc_state(TackGCState new_state) {
    state = new_state;
}
TackGCState Heap::gc_state() const {
    return state;
}

TackValue::ArrayType* Heap::alloc_array() {
    alloc_count++;
    return &arrays.emplace_back();
}

TackValue::ObjectType* Heap::alloc_object() {
    alloc_count++;
    return &objects.emplace_back();
}

TackValue::FunctionType* Heap::alloc_function(CodeFragment* code) {
    alloc_count++;
    return &functions.emplace_back(TackValue::FunctionType {
        .bytecode = code,
        .captures = {}
    });
}

BoxType* Heap::alloc_box(TackValue val) {
    alloc_count++;
    return &boxes.emplace_back(BoxType { .value = val });
}

TackValue::StringType* Heap::alloc_string(const std::string& data) {
    alloc_count++;
    return &strings.emplace_back(TackValue::StringType { data });
}

void gc_visit(TackValue value);

void gc_visit(TackValue::StringType* str) {
    str->marker = true;
}
void gc_visit(BoxType* box) {
    if (!box->marker) {
        box->marker = true;
        gc_visit(box->value);
    }
}
void gc_visit(TackValue::ObjectType* obj) {
    if (!obj->marker) {
        obj->marker = true;
        for (auto i = obj->data.begin(); i != obj->data.end(); i = obj->data.next(i)) {
            gc_visit(obj->data.value_at(i));
        }
    }
}
void gc_visit(TackValue::ArrayType* arr) {
    if (!arr->marker) {
        arr->marker = true;
        for (auto v : arr->data) {
            gc_visit(v);
        }
    }
}
void gc_visit(TackValue::FunctionType* func) {
    if (!func->marker) {
        func->marker = true;
        for (auto v : func->captures) {
            gc_visit(v);
        }
    }
}

void gc_visit(TackValue value) {
    // dump("visit: ", value);
    switch ((uint64_t)value.get_type()) {
        case (uint64_t)TackType::String: return gc_visit(value.string());
        case (uint64_t)TackType::Object: return gc_visit(value.object());
        case (uint64_t)TackType::Array: return gc_visit(value.array());
        case (uint64_t)TackType::Function: return gc_visit(value.function());
        case type_bits_boxed: return gc_visit(value_to_boxed(value));
        default:break;
    }
}

void Heap::gc(std::vector<TackValue>& globals, const Stack &stack, uint32_t stackbase) {
    // Basic mark-n-sweep garbage collector
    // TODO: improve code style everywhere
    if (state == TackGCState::Disabled
        || alloc_count < prev_alloc_count * 2 
        || alloc_count <= MIN_GC_ALLOCATIONS) {
        return;
    }

    // gc every 1ms
    auto before = std::chrono::steady_clock::now();
    debug("===== GC: START ===");
    debug("  prev alloc count: ", prev_alloc_count);
    debug("  cur alloc count:  ", alloc_count);
    debug("  last gc:          ", last_gc.time_since_epoch().count());
    auto num_collections = 0;

    // visit globals
    for (const auto& v: globals) {
        gc_visit(v);
    }

    // visit stack
    // assume this is being called from a return site, so the contents of the stack above stackbase are no longer needed
    // visiting the stack frame data (return pc, etc) seems messy but is intentional - we should visit the functions in the call stack anyway
    for (auto i = 0u; i < stackbase; i++) {
        gc_visit(stack[i]);
    }

    // visit any refcounted functions, objects, arrays
    // TODO: inefficient? might be better to have a separate list
    // and transfer objects to it when refcount > 0
    // can use std::list::splice for this
    for (auto& o : objects) {
        if (o.refcount) {
            gc_visit(&o);
        }
    }
    for (auto& a : arrays) {
        if (a.refcount) {
            gc_visit(&a);
        }
    }
    for (auto& f : functions) {
        if (f.refcount) {
            gc_visit(&f);
        }
    }


    // "sweep up" (ie deallocate) anything that wasn't visited
    dump("sweeping strings");
    {
        auto i = strings.begin();
        while (i != strings.end()) {
            if (!i->marker && i->refcount == 0) {
                dump("collected string: ", TackValue::string(&(*i)));
                num_collections += 1;
                i = strings.erase(i);
            } else {
                i->marker = false;
                i++;
            }
        }
    }

    dump("sweeping objects");
    {
        auto i = objects.begin();
        while (i != objects.end()) {
            if (!i->marker && i->refcount == 0) {
                dump("collected object: ", TackValue::object(&(*i)));
                num_collections += 1;
                i = objects.erase(i);
            } else {
                i->marker = false;
                i++;
            }
        }
    }

    dump("sweeping arrays");
    {
        auto i = arrays.begin();
        while (i != arrays.end()) {
            if (!i->marker && i->refcount == 0) {
                dump("collected array: ", TackValue::array(&(*i)));
                num_collections += 1;
                i = arrays.erase(i);
            } else {
                i->marker = false;
                i++;
            }
        }
    }

    dump("sweeping boxes");
    {
        auto i = boxes.begin();
        while (i != boxes.end()) {
            if (!i->marker && i->refcount == 0) {
                dump("collected box", i->value);
                num_collections += 1;
                i = boxes.erase(i);
            } else {
                i->marker = false;
                i++;
            }
        }
    }

    dump("sweeping functions");
    {
        auto i = functions.begin();
        while (i != functions.end()) {
            if (!i->marker && i->refcount == 0) {
                dump("collected function: ", i->bytecode->name);
                num_collections += 1;
                i = functions.erase(i);
            } else {
                i->marker = false;
                i++;
            }
        }
    }

    auto after = std::chrono::steady_clock::now();
    last_gc = after;
    auto duration = after - before;
    auto ms = (float)std::chrono::duration_cast<std::chrono::microseconds>(duration).count() / 1000.f;
    last_gc_ms = ms;
    prev_alloc_count = alloc_count;
    alloc_count -= num_collections;
    debug("===== GC: END =====");
    debug("  collected:       ", num_collections);
    debug("  new alloc count: ", alloc_count);
    debug("  time taken (ms): ", ms);

}
