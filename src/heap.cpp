#include "value.h"
#include "interpreter.h"
#include <jlib/log.h>

// TODO: proper debug logging / diagnostics / monitoring system
#define debug log
// #define debug(...)
#define dump(...)

void Heap::gc_state(GCState new_state) {
    state = new_state;
}
GCState Heap::gc_state() const {
    return state;
}

ArrayType* Heap::alloc_array() {
    alloc_count++;
    return &arrays.emplace_back();
}

ObjectType* Heap::alloc_object() {
    alloc_count++;
    return &objects.emplace_back();
}

FunctionType* Heap::alloc_function(CodeFragment* code) {
    alloc_count++;
    return &functions.emplace_back(FunctionType {
        .bytecode = code,
        .captures = {}
    });
}

BoxType* Heap::alloc_box(Value val) {
    alloc_count++;
    return &boxes.emplace_back(BoxType { .value = val });
}

StringType* Heap::alloc_string(const std::string& data) {
    alloc_count++;
    return &strings.emplace_back(StringType { data });
}

void gc_visit(Value value) {
    dump("visit: ", value);
    switch ((uint64_t)value_get_type(value)) {
        case type_bits_string: {
            auto s = value_to_string(value);
            s->marker = true;
            break;
        }
        case type_bits_boxed: {
            auto b = value_to_boxed(value);
            if (!b->marker) {
                b->marker = true;
                gc_visit(b->value);
            }
            break;
        }
        case type_bits_object: {
            auto o = value_to_object(value);
            if (!o->marker) {
                o->marker = true;
                for (auto i = o->begin(); i != o->end(); i = o->next(i)) {
                    gc_visit(o->value_at(i));
                }
            }
            break;
        }
        case type_bits_array: {
            auto a = value_to_array(value);
            if (!a->marker) {
                a->marker = true;
                for (auto v: *a) {
                    gc_visit(v);
                }
            }
            break;
        }
        case type_bits_function: {
            auto f = value_to_function(value);
            if (!f->marker) {
                f->marker = true;
                for (auto v: f->captures) {
                    gc_visit(v);
                }
            }
            break;
        }
        default:break;
    }
}

void Heap::gc(std::vector<Value>& globals, const Stack &stack, uint32_t stackbase) {
    // Basic mark-n-sweep garbage collector
    // Doesn't handle strings just yet
    // TODO: bad code style everywhere
    if (state == GCState::Disabled
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

    // mark globals
    for (const auto& v: globals) {
        gc_visit(v);
    }

    // mark stack
    gc_visit(stack[stackbase - STACK_FRAME_OVERHEAD]); // visit return value
    for (auto s = stackbase; stack[s-1]._i != 0; s = stack[s-1]._i) {
        for (auto i = stack[s-1]._i; i < s - (STACK_FRAME_OVERHEAD); i++) {
            gc_visit(stack[i]);
        }
    }

    dump("sweeping strings");
    {
        auto i = strings.begin();
        while (i != strings.end()) {
            if (!i->marker && i->refcount == 0) {
                dump("collected string: ", value_from_string(&(*i)));
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
                dump("collected object: ", value_from_object(&(*i)));
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
                dump("collected array: ", *i);
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
