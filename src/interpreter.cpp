#include "interpreter.h"
#include "compiler.h"
#include "value.h"
#include "interpreter.h"
#include "parsing.h"

#define JLIB_LOG_VISUALSTUDIO
#include <jlib/log.h>
#include <jlib/text_file.h>

#include <list>
#include <vector>
#include <array>
#include <exception>
#include <cstring>
#include <filesystem>
#include <signal.h>

#include "khash2.h"

using namespace std::string_literals;

static const std::string GLOBAL_NAMESPACE = "[global]";

Interpreter::Interpreter() {
    next_globalid = 0;
    stackbase = 0;

    // create the true global scope
    global_scope.compiler = nullptr;
    global_scope.parent_scope = nullptr;
    global_scope.is_function_scope = false;
    modules.value_at(modules.put(GLOBAL_NAMESPACE)) = &global_scope;

    stack.fill(value_null());
}
Interpreter::~Interpreter() {}

void* Interpreter::get_user_pointer() const {
    return user_pointer;
}

void Interpreter::set_user_pointer(void* ptr) {
    user_pointer = ptr;
}

uint16_t Interpreter::next_gid() {
    if (next_globalid == UINT16_MAX) {
        throw std::runtime_error("too many globals");
    }
    return next_globalid++;
}

void Interpreter::gc_state(GCState state) {
    heap.gc_state(state);
}
ArrayType* Interpreter::alloc_array() {
    return heap.alloc_array();
}
ObjectType* Interpreter::alloc_object() {
    return heap.alloc_object();
}
FunctionType* Interpreter::alloc_function(CodeFragment* code) {
    return heap.alloc_function(code);
}
BoxType* Interpreter::alloc_box(Value val) {
    return heap.alloc_box(val);
}
StringType* Interpreter::intern_string(const std::string& data) {
    auto got = key_cache.find(data);
    if (got == key_cache.end()) {
        auto put = key_cache.put(data);
        auto str = new StringType { data };
        key_cache.value_at(put) = str;
        return str;
    }
    return key_cache.value_at(got);
}
StringType* Interpreter::alloc_string(const std::string& data) {
    return heap.alloc_string(data);
}
GCState Interpreter::gc_state() const {
    return heap.gc_state();
}

Value Interpreter::get_global(const std::string& name) {
    return get_global(name, GLOBAL_NAMESPACE);
}
Value Interpreter::get_global(const std::string& name, const std::string& module_name) {
    auto mod = modules.find(module_name);
    if (mod == modules.end()) {
        return value_null();
    }
    auto* module = modules.value_at(mod);

    auto* var = module->lookup(name);
    if (!var) {
        return value_null();
    }

    return globals[var->g_id];
}

Compiler::VariableContext* Interpreter::set_global(const std::string& name, bool is_const, Value value) {
    return set_global(name, GLOBAL_NAMESPACE, is_const, value);
}

Compiler::VariableContext* Interpreter::set_global(const std::string& name, const std::string& module_name, bool is_const, Value value) {
    auto iter = modules.find(module_name);
    if (iter == modules.end()) {
        log("Error: module not found:", module_name);
        return nullptr;
    }
    auto scope = modules.value_at(iter);

    // lookup binding
    auto* var = scope->lookup(name);

    // create binding if not exist
    if (!var) {
        // HACK:
        var = &scope->bindings.try_emplace(name, Compiler::VariableContext {
            .reg = 0xff,
            .is_const = is_const,
            .is_global = true,
            .g_id = next_gid()
        }).first->second;
    }

    // size check
    if (globals.size() <= var->g_id) {
        globals.resize(var->g_id + 1);
    }

    // write initial value (useful for C; when a compiler does this, it will emit WRITE_GLOBAL with the true value)
    globals[var->g_id] = value;

    return var;
}

CodeFragment* Interpreter::create_fragment() {
    return &fragments.emplace_back();
}

void Interpreter::add_module_dir_cwd() {
    module_dirs.push_back(std::filesystem::current_path().string());
}
void Interpreter::add_module_dir(const std::string& dir) {
    module_dirs.push_back(dir);
}

Compiler::ScopeContext* Interpreter::load_module(const std::string& module_name) {
    auto iter = modules.find(module_name);
    if (iter == modules.end()) {
        auto filename = module_name + ".tack";
        auto file_data = read_text_file(filename);
        for (auto& d : module_dirs) {
            auto path = std::filesystem::path(d).append(filename).string();
            file_data = read_text_file(std::filesystem::path(d).append(filename).string());
            if (file_data.has_value()) {
                break;
            }
        }
        if (!file_data.has_value()) {
            log("Unable to load module: ", module_name);
            return nullptr;
        }
    
        // parse
        auto out_ast = AstNode {};
        parse(file_data.value(), out_ast);
        auto ast = AstNode(AstType::FuncLiteral, AstNode(AstType::ParamDef), out_ast);
        if (log_ast) {
            log(ast.tostring());
        }

        // create the top level scope for the module
        auto scope = new Compiler::ScopeContext {};
        scope->compiler = nullptr;
        scope->parent_scope = &global_scope;
        scope->is_function_scope = false;
        modules.value_at(modules.put(module_name)) = scope;

        // compile root fragment
        auto compiler = Compiler { .interpreter = this };
        auto* fragment = create_fragment();
        fragment->name = module_name;
        compiler.compile_func(&ast, fragment, scope);
        auto* func = heap.alloc_function(fragment);

        // immediately call function
        func->refcount += 1; // HACK:
        call(value_from_function(func), 0, nullptr);
        func->refcount -= 1;

        return scope;

    } else {
        // already loaded
        // log("Module already loaded: ", module_name, " - already loaded");
        return modules.value_at(iter);
    }
    
}

#define handle(opcode)  break; case Opcode::opcode:
#define error(err)      throw std::runtime_error(err);
#define REGISTER_RAW(n) stack[stackbase+n]
#define REGISTER(n)     *(value_is_boxed(REGISTER_RAW(n)) ? &value_to_boxed(REGISTER_RAW(n))->value : &REGISTER_RAW(n))

Value Interpreter::call(Value fn, int nargs, Value* args) {
    auto* func = value_to_function(fn);

    // program counter
    auto _pr = func; // current program
    auto _pc = 0u;
    auto _pe = func->bytecode->instructions.size();

    // stack/registers
    auto initial_stackbase = stackbase;
    stackbase += STACK_FRAME_OVERHEAD;

    // copy arguments to stack
    if (nargs && args != &stack[stackbase]) {
        std::memcpy(&stack[stackbase], args, sizeof(Value) * nargs);
    }

    // set up initial call frame
    REGISTER_RAW(-3)._i = _pr->bytecode->instructions.size();
    REGISTER_RAW(-2)._p = (void*)func;
    REGISTER_RAW(-1)._i = initial_stackbase; // special case

    auto dumpstack = [&]() {
        auto ln = _pr->bytecode->line_numbers[_pc];
        log("encountered at line:", ln, "in", _pr->bytecode->name);
        /*auto s = stackbase;
        while (stack[s - 1]._i != 0) {
            auto retpc = stack[s - 3]._i;
            auto func = ((FunctionType*)stack[s - 2]._p)->bytecode;
            auto retln = func->line_numbers[retpc];
            log(" - called from line:", retln, "in", func->name);
            s = stack[s - 1]._i;
        }*/
    };

    try {
        while (_pc < _pe) {
            auto i = _pr->bytecode->instructions[_pc];
            switch (i.opcode) {
            case Opcode::UNKNOWN: break;
                handle(ZERO_CAPTURE) {
                    REGISTER_RAW(i.r0) = value_null();
                }
                handle(LOAD_I_SN) {
                    REGISTER(i.r0) = value_from_number(i.u1);
                }
                handle(LOAD_I_NULL) {
                    REGISTER(i.r0) = value_null();
                }
                handle(LOAD_I_BOOL) {
                    REGISTER(i.r0) = value_from_boolean(i.u8.r1);
                }
                handle(LOAD_CONST) {
                    REGISTER(i.r0) = _pr->bytecode->storage[i.u1];
                }
                handle(INCREMENT) {
                    REGISTER(i.r0) = value_from_number(value_to_number(REGISTER(i.r0)) + 1);
                }
                handle(ADD) {
                    auto lhs = REGISTER(i.u8.r1);
                    auto lt = value_get_type(lhs);
                    if (lt == Type::Number) {
                        // numeric add
                        REGISTER(i.r0) = value_from_number(value_to_number(lhs) + value_to_number(REGISTER(i.u8.r2)));
                    } else if (lt == Type::String) {
                        // string add
                        auto l = value_to_string(lhs);
                        auto r = value_to_string(REGISTER(i.u8.r2));
                        auto new_str = std::string(l->data) + std::string(r->data);
                        REGISTER(i.r0) = value_from_string(alloc_string(new_str));
                    } else if (lt == Type::Array) {
                        // array add
                        auto l = value_to_array(lhs);
                        auto r = value_to_array(REGISTER(i.u8.r2));
                        auto n = alloc_array();
                        std::copy(l->begin(), l->end(), std::back_inserter(*n));
                        std::copy(r->begin(), r->end(), std::back_inserter(*n));
                    } else {
                        error("operator '+' exected number / array / string");
                    }
                }
                handle(SUB) {
                    REGISTER(i.r0) = value_from_number(value_to_number(REGISTER(i.u8.r1)) - value_to_number(REGISTER(i.u8.r2)));
                }
                handle(MUL) {
                    REGISTER(i.r0) = value_from_number(value_to_number(REGISTER(i.u8.r1)) * value_to_number(REGISTER(i.u8.r2)));
                }
                handle(DIV) {
                    REGISTER(i.r0) = value_from_number(value_to_number(REGISTER(i.u8.r1)) / value_to_number(REGISTER(i.u8.r2)));
                }
                handle(MOD) {
                    auto x = value_to_number(REGISTER(i.u8.r1));
                    auto y = value_to_number(REGISTER(i.u8.r2));
                    REGISTER(i.r0) = value_from_number(fmod(x, y));
                }
                handle(SHL) {
                    auto lhs = REGISTER(i.u8.r1);
                    auto rhs = REGISTER(i.u8.r2);
                    if (value_is_array(lhs)) {
                        auto* arr = value_to_array(lhs);
                        arr->emplace_back(rhs);
                        // put the appended value into r0
                        REGISTER(i.r0) = rhs;
                    } else if (value_is_number(lhs)) {
                        REGISTER(i.r0) = value_from_number(
                            (uint32_t)value_to_number(lhs) << 
                            (uint32_t)value_to_number(rhs)
                        );
                    } else {
                        error("expected number or array");
                    }
                }
                handle(SHR) {
                    auto lhs = REGISTER(i.u8.r1);
                    auto rhs = REGISTER(i.u8.r2);
                    if (value_is_array(lhs)) {
                        auto* arr = value_to_array(lhs);
                        // put the popped value into r0
                        REGISTER(i.r0) = arr->back();
                        arr->pop_back();
                    } else if (value_is_number(lhs)) {
                        REGISTER(i.r0) = value_from_number(
                            (uint32_t)value_to_number(lhs) >>
                            (uint32_t)value_to_number(rhs)
                        );
                    } else {
                        error("expected number or array");
                    }
                }
                
                handle(AND) {
                    REGISTER(i.r0) = value_from_boolean(
                        value_get_truthy(REGISTER(i.u8.r1)) && value_get_truthy(REGISTER(i.u8.r2))
                    );
                }
                handle(OR) {
                    REGISTER(i.r0) = value_from_boolean(
                        value_get_truthy(REGISTER(i.u8.r1)) || value_get_truthy(REGISTER(i.u8.r2))
                    );
                }
                
                handle(EQUAL) {
                    REGISTER(i.r0) = value_from_boolean(REGISTER(i.u8.r1) == REGISTER(i.u8.r2));
                }
                handle(NEQUAL) {
                    REGISTER(i.r0) = value_from_boolean(REGISTER(i.u8.r1) != REGISTER(i.u8.r2));
                }
                handle(LESS) {
                    REGISTER(i.r0) = value_from_boolean(
                        value_to_number(REGISTER(i.u8.r1)) <
                        value_to_number(REGISTER(i.u8.r2))
                    );
                }
                handle(LESSEQ) {
                    REGISTER(i.r0) = value_from_boolean(
                        value_to_number(REGISTER(i.u8.r1)) <=
                        value_to_number(REGISTER(i.u8.r2))
                    );
                }
                handle(GREATER) {
                    REGISTER(i.r0) = value_from_boolean(
                        value_to_number(REGISTER(i.u8.r1)) >
                        value_to_number(REGISTER(i.u8.r2))
                    );
                }
                handle(GREATEREQ) {
                    REGISTER(i.r0) = value_from_boolean(
                        value_to_number(REGISTER(i.u8.r1)) >=
                        value_to_number(REGISTER(i.u8.r2))
                    );
                }
                handle(MOVE) {
                    REGISTER(i.r0) = REGISTER(i.u8.r1);
                }
                handle(READ_GLOBAL) {
                    REGISTER(i.r0) = globals[i.u1]; // i.u1 will never be out of range
                }
                handle(WRITE_GLOBAL) {
                    globals[i.u1] = REGISTER(i.r0); // i.u1 will never be out of range
                }
                handle(FOR_INT) {
                    auto var = value_to_number(REGISTER(i.r0));
                    auto end = value_to_number(REGISTER(i.u8.r1));
                    if (var < end) {
                        _pc++;
                    }
                }
                handle(FOR_ITER_INIT) {
                    auto iter_val = REGISTER(i.u8.r1);
                    auto iter_type = value_get_type(iter_val);
                    if (iter_type == Type::Array) {
                        REGISTER_RAW(i.r0)._i = 0;
                    } else if (iter_type == Type::Object) {
                        REGISTER_RAW(i.r0)._i = value_to_object(iter_val)->begin();
                    // } else if (iter_type == Type::Function) {
                    } else {
                        error("for loop expected array or object");
                    }
                }
                handle(FOR_ITER) {
                    auto iter_val = REGISTER(i.u8.r1);
                    auto iter_type = value_get_type(iter_val);
                    if (iter_type == Type::Array) {
                        auto ind = REGISTER_RAW(i.r0)._i;
                        auto arr = value_to_array(iter_val);
                        if (ind < arr->size()) {
                            REGISTER(i.u8.r2) = (*arr)[ind];
                            _pc++;
                        }
                    } else if (iter_type == Type::Object) {
                        auto it = REGISTER_RAW(i.r0)._i;
                        auto obj = value_to_object(iter_val);
                        if (it != obj->end()) {
                            auto key = obj->key_at(it);
                            auto cached = key_cache.value_at(key_cache.find(key));
                            REGISTER(i.u8.r2) = value_from_string(cached);
                            _pc++;
                        }
                    // } else if (iter_type == Type::Function) {
                    }
                }
                handle(FOR_ITER2) {
                    auto iter_val = REGISTER(i.u8.r1);
                    auto obj = value_to_object(iter_val);
                    auto it = REGISTER_RAW(i.r0)._i;
                    if (it != obj->end()) {
                        auto key = obj->key_at(it);
                        auto cached = key_cache.value_at(key_cache.find(key));
                        REGISTER(i.u8.r2) = value_from_string(cached);
                        REGISTER(i.u8.r2 + 1) = obj->value_at(it);
                        _pc++;
                    }
                }
                handle(FOR_ITER_NEXT) {
                    auto iter_val = REGISTER(i.u8.r1);
                    auto iter_type = value_get_type(iter_val);
                    if (iter_type == Type::Array) {
                        // increment index
                        REGISTER_RAW(i.r0)._i += 1;
                    } else if (iter_type == Type::Object) {
                        // next key
                        auto obj = value_to_object(iter_val);
                        REGISTER_RAW(i.r0)._i = obj->next(REGISTER_RAW(i.r0)._i);
                    }
                }
                handle(CONDSKIP) {
                    if (value_get_truthy(REGISTER(i.r0))) {
                        _pc++;
                    }
                }
                handle(JUMPF) { _pc += i.u1 - 1; }
                handle(JUMPB) { _pc -= i.u1 + 1; }
                handle(LEN) {
                    auto val = REGISTER(i.u8.r1);
                    auto type = value_get_type(val);
                    if (type == Type::Array) {
                        REGISTER(i.r0) = value_from_number(value_to_array(val)->size());
                    } else if (type == Type::String) {
                        REGISTER(i.r0) = value_from_number(value_to_string(val)->data.size());
                    } else if (type == Type::Object) {
                        REGISTER(i.r0) = value_from_number(value_to_object(val)->size());
                    } else {
                        error("operator '#' expected string / array / object");
                    }
                }
                handle(NEGATE) {
                    REGISTER(i.r0) = value_from_number(-value_to_number(REGISTER(i.u8.r1)));
                }
                handle(READ_CAPTURE) {
                    REGISTER_RAW(i.r0) = _pr->captures[i.u8.r1];
                }
                handle(ALLOC_FUNC) {
                    // create closure
                    auto code = (CodeFragment*)value_to_pointer(_pr->bytecode->storage[i.u1]);
                    auto* func = heap.alloc_function(code);
                    // capture captures; box inplace if necessary
                    for (const auto& c : code->capture_info) {
                        auto val = REGISTER_RAW(c.source_register);
                        if (!value_is_boxed(val)) {
                            auto box = value_from_boxed(heap.alloc_box(val));
                            func->captures.emplace_back(box);
                            REGISTER_RAW(c.source_register) = box;
                        } else {
                            func->captures.emplace_back(val);
                        }
                    }
                    // done
                    REGISTER(i.r0) = value_from_function(func);
                }
                handle(ALLOC_ARRAY) {
                    auto* arr = heap.alloc_array();
                    // emplace child elements
                    for (auto e = 0; e < i.u8.r1; e++) {
                        arr->emplace_back(REGISTER(i.u8.r2 + e));
                    }
                    REGISTER(i.r0) = value_from_array(arr);
                }
                handle(ALLOC_OBJECT) {
                    auto* obj = heap.alloc_object();
                    // emplace child elements
                    for (auto e = 0; e < i.u8.r1; e++) {
                        auto key = value_to_string(REGISTER(i.u8.r2 + e * 2));
                        auto val = REGISTER(i.u8.r2 + e * 2 + 1);
                        obj->set(key->data, val);
                    }
                    REGISTER(i.r0) = value_from_object(obj);
                }
                handle(LOAD_ARRAY) {
                    auto arr_val = REGISTER(i.u8.r1);
                    auto ind_val = REGISTER(i.u8.r2);
                    auto* arr = value_to_array(arr_val);
                    auto ind = value_to_number(ind_val);
                    if (ind >= arr->size() || ind < 0) {
                        error("index out of range");
                    }
                    REGISTER(i.r0) = (*arr)[ind];
                }
                handle(STORE_ARRAY) {
                    auto arr_val = REGISTER(i.u8.r1);
                    auto ind_val = REGISTER(i.u8.r2);
                    auto* arr = value_to_array(arr_val);
                    auto ind = value_to_number(ind_val);
                    if (ind > arr->size()) {
                        error("index out of range");
                    }
                    (*arr)[ind] = REGISTER(i.r0);
                }
                handle(LOAD_OBJECT) {
                    auto lhs = REGISTER(i.u8.r1);
                    auto key = value_to_string(REGISTER(i.u8.r2));
                    auto* obj = value_to_object(lhs);
                    auto found = false;
                    auto val = obj->get(key->data, found);
                    if (!found) {
                        error("key not found"s + key->data);
                    } else {
                        REGISTER(i.r0) = val;
                    }
                }
                handle(STORE_OBJECT) {
                    auto lhs = REGISTER(i.u8.r1);
                    auto key_val = REGISTER(i.u8.r2);
                    auto* obj = value_to_object(lhs);
                    auto key = value_to_string(key_val);
                    obj->set(key->data, REGISTER(i.r0));
                }
                handle(CALL) {
                    auto r0 = REGISTER(i.r0);
                    if (value_is_function(r0)) {
                        auto func = value_to_function(r0);
                        auto nargs = i.u8.r1;
                        auto correct_nargs = 0; // func->bytecode->nargs;
                        // TODO: arity checking
                        if (false && nargs != correct_nargs) {
                            error("wrong nargs");
                        }
                        auto new_base = i.u8.r2 + STACK_FRAME_OVERHEAD;

                        if (new_base + func->bytecode->max_register > MAX_STACK) {
                            error("would exceed stack");
                        }
                        
                        // set up call frame
                        REGISTER_RAW(new_base - 3)._i = _pc; // push return addr
                        REGISTER_RAW(new_base - 2)._p = (void*)_pr; // return program
                        REGISTER_RAW(new_base - 1)._i = stackbase; // push return frameptr

                        _pr = func;
                        _pe = func->bytecode->instructions.size();
                        _pc = -1;
                        stackbase = stackbase + new_base; // new stack frame
                    } else if (value_is_cfunction(r0)) {
                        auto cfunc = value_to_cfunction(r0);
                        auto nargs = i.u8.r1;
                        
                        auto old_base = stackbase;
                        stackbase = stackbase + i.u8.r2 + STACK_FRAME_OVERHEAD;                        
                        auto retval = cfunc(this, nargs, &stack[stackbase]);
                        stackbase = old_base;
                        REGISTER_RAW(i.u8.r2) = retval;
                    } else {
                        error("tried to call non-function");
                    }
                }
                handle(RET) {
                    auto return_addr = REGISTER_RAW(-3);
                    auto return_func = REGISTER_RAW(-2);
                    auto return_stack = REGISTER_RAW(-1);
                    auto return_val = REGISTER(i.u8.r1);

                    _pc = return_addr._i;
                    _pr = (FunctionType*)return_func._p;
                    _pe = _pr->bytecode->instructions.size();

                    // "Clean" the stack - Must not leave any boxes in unused registers or subsequent loads to register will mistakenly write-through
                    std::memset(stack.data() + stackbase, 0xffffffff, MAX_REGISTERS * sizeof(Value));
                    
                    REGISTER_RAW(-3) = return_val;
                    REGISTER_RAW(-2) = value_null();
                    REGISTER_RAW(-1) = value_null();
                    heap.gc(globals, stack, stackbase);
                    
                    stackbase = return_stack._i;
                    if (stackbase == initial_stackbase) {
                        return return_val;
                    }
                }
            break; default: error("unknown instruction: " + to_string(i.opcode));
            }
            _pc++;
        }
    } catch (std::exception& e) {
        log("runtime error: ", e.what());
        dumpstack();
        return value_null();
    }

    error("Reached end of interpreter loop - should be impossible");
    return value_null(); // should be unreachable
}

#undef error
#undef handle
#undef REGISTER
#undef REGISTER_RAW
