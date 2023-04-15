#include "value.h"
#include "interpreter.h"

ArrayType* Heap::alloc_array() {
    return &_arrays.emplace_back();
}

ObjectType* Heap::alloc_object() {
    return &_objects.emplace_back();
}

FunctionType* Heap::alloc_function(CodeFragment* code) {
    return &_functions.emplace_back(FunctionType {
        .bytecode = code,
        .captures = alloc_array()
    });
}

BoxType* Heap::alloc_box(Value val) {
    return &_boxes.emplace_back(BoxType {
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
                f->captures->marker = true;
                for (auto v: f->captures->values) {
                    gc_visit(v);
                }
            }
            break;
        }
    }
}

void Heap::gc(std::vector<Value>& globals, const StackType &stack, uint32_t stackbase) {
    // Basic mark-n-sweep garbage collector
    // Doesn't handle strings just yet

    // gc every 1ms
    auto now = std::chrono::steady_clock::now();
    if (1000 > std::chrono::duration_cast<std::chrono::microseconds>(now - last_gc).count()) {
        return;
    }
    last_gc = now;

    // mark globals
    for (const auto& v: globals) {
        gc_visit(v);
    }

    // mark stack
    gc_visit(stack[stackbase-4]); // special case: s-4 contains the return value (unique to the current stack frame) // TODO: remove this
    for (auto s = stackbase; stack[s-2]._i != 0; s = stack[s-2]._i) {
        for (auto i = stack[s-2]._i; i < s - 4; i++) {
            gc_visit(stack[i]);
        }
    }

    // sweep objects
    auto i = _objects.begin();
    while (i != _objects.end()) {
        if (!i->marker && i->refcount == 0) {
            // std::cout << "collected " << value_from_object(&(*i)) << std::endl;
            i = _objects.erase(i);
        } else {
            i->marker = false;
            i++;
        }
    }

    // sweep arrays
    auto j = _arrays.begin();
    while (j != _arrays.end()) {
        if (!j->marker && j->refcount == 0) {
            // std::cout << "collected " << value_from_array(&(*j)) << std::endl;
            j = _arrays.erase(j);
        } else {
            j->marker = false;
            j++;
        }
    }

    // sweep boxes
    auto k = _boxes.begin();
    while (k != _boxes.end()) {
        if (!k->marker && k->refcount == 0) {
            // std::cout << "collected " << value_from_boxed(&(*k)) << std::endl;
            k = _boxes.erase(k);
        } else {
            k->marker = false;
            k++;
        }
    }

    // sweep functions
    auto f = _functions.begin();
    while (f != _functions.end()) {
        if (!f->marker && f->refcount == 0) {
            // std::cout << "collected " << value_from_function(&(*f)) << std::endl;
            f = _functions.erase(f);
        } else {
            f->marker = false;
            f++;
        }
    }

    now = std::chrono::steady_clock::now();
    last_gc_duration = now - last_gc;
}
