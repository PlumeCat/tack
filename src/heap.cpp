#include "value.h"
#include "interpreter.h"
#include <jlib/log.h>

// TODO: proper debug logging / diagnostics / monitoring system

#define log(...)

ArrayType* Heap::alloc_array() {
    return &arrays.emplace_back();
}

ObjectType* Heap::alloc_object() {
    return &objects.emplace_back();
}

FunctionType* Heap::alloc_function(CodeFragment* code) {
    return &functions.emplace_back(FunctionType {
        .bytecode = code,
        .captures = {}
    });
}

BoxType* Heap::alloc_box(Value val) {
    return &boxes.emplace_back(BoxType {
        .value = val
    });
}

void gc_visit(Value value) {
    switch (value_get_type(value)) {
        case (Type)type_bits_boxed: {
            auto b = value_to_boxed(value);
            if (!b->marker) {
                b->marker = true;
                gc_visit(b->value);
            }
            break;
        }
        case Type::Object: {
            auto o = value_to_object(value);
            if (!o->marker) {
                o->marker = true;
                for (auto i = o->begin(); i != o->end(); i = o->next(i)) {
                    gc_visit(o->value(i));
                }
            }
            break;
        }
        case Type::Array: {
            auto a = value_to_array(value);
            if (!a->marker) {
                a->marker = true;
                for (auto v: a->values) {
                    gc_visit(v);
                }
            }
            break;
        }
        case Type::Function: {
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

    // TODO: have a look at incremental GC
    // TODO: long way down the line, mark phase should be parallelizable
    // https://www.lua.org/wshop18/Ierusalimschy.pdf

    // TODO: look into generational hypothesis
    // TODO: general speed ups

    // gc every 1ms
    auto now = std::chrono::steady_clock::now();
    if (1000 > std::chrono::duration_cast<std::chrono::microseconds>(now - last_gc).count()) {
    // if (true) {
        return;
    }
    last_gc = now;

    log("===== GC =====");

    // mark globals
    for (const auto& v: globals) {
        gc_visit(v);
    }

    // TODO:
    // need to consider refcounted stuff as GC roots, so the C-side can keep stuff alive
    // Lua solves this with the registry, maybe that's faster? separate pool for objects referenced from C
    // remove from tack-pool when refcounted, return to tack-pool when released
    // iterate over all allocations, and visit when there's a nonzero refcount

    // mark stack
    // TODO: visit functions in the call stack just in case
    // TODO: remove this special case: also need to visit the return value (unique to the current call frame)
    gc_visit(stack[stackbase - STACK_FRAME_OVERHEAD]);
    for (auto s = stackbase; stack[s-1]._i != 0; s = stack[s-1]._i) {
        for (auto i = stack[s-1]._i; i < s - (STACK_FRAME_OVERHEAD); i++) {
            gc_visit(stack[i]);
        }
    }

    log("sweeping objects");
    {
        auto i = objects.begin();
        while (i != objects.end()) {
            if (!i->marker && i->refcount == 0) {
                log("collected object: ", value_from_object(&(*i)));
                i = objects.erase(i);
            } else {
                i->marker = false;
                i++;
            }
        }
    }

    log("sweeping arrays");
    {
        auto i = arrays.begin();
        while (i != arrays.end()) {
            if (!i->marker && i->refcount == 0) {
                log("collected array: ", *i);
                i = arrays.erase(i);
            } else {
                i->marker = false;
                i++;
            }
        }
    }

    log("sweeping boxes");
    {
        auto i = boxes.begin();
        while (i != boxes.end()) {
            if (!i->marker && i->refcount == 0) {
                log("collected box", i->value);
                i = boxes.erase(i);
            } else {
                i->marker = false;
                i++;
            }
        }
    }

    log("sweeping functions");
    {
        auto i = functions.begin();
        while (i != functions.end()) {
            if (!i->marker && i->refcount == 0) {
                log("collected function: ", i->bytecode->name);
                i = functions.erase(i);
            } else {
                i->marker = false;
                i++;
            }
        }
    }
    
    #undef sweep

    log("===== ENDGC =====");

    now = std::chrono::steady_clock::now();
    last_gc_duration = now - last_gc;
}
