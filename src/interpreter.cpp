#include "interpreter.h"
#include "compiler.h"
#include "interpreter.h"
#include "parsing.h"

#include <list>
#include <vector>
#include <array>
#include <exception>
#include <cstring>
#include <filesystem>
#include <optional>

#include "khash2.h"

using namespace std::string_literals;

static const std::string GLOBAL_NAMESPACE = "[global]";

std::optional<std::string> read_text_file(const std::string& fname);

TackVM* TackVM::create() {
    return new Interpreter();
}

Interpreter::Interpreter() {
    srand(time(nullptr)); // TODO: remove
    next_globalid = 0;
    stackbase = 0;

    // create the true global scope
    global_scope.compiler = nullptr;
    global_scope.parent_scope = nullptr;
    global_scope.is_function_scope = false;
    modules.value_at(modules.put(GLOBAL_NAMESPACE)) = &global_scope;

    stack.fill(TackValue::null());
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
        error("too many globals");
    }
    return next_globalid++;
}

void Interpreter::set_gc_state(TackGCState state) {
    heap.gc_state(state);
}
TackGCState Interpreter::get_gc_state() const {
    return heap.gc_state();
}
TackValue::ArrayType* Interpreter::alloc_array() {
    return heap.alloc_array();
}
TackValue::ObjectType* Interpreter::alloc_object() {
    return heap.alloc_object();
}
TackValue::FunctionType* Interpreter::alloc_function(CodeFragment* code) {
    return heap.alloc_function(code);
}
TackValue::FunctionType* Interpreter::alloc_function(TackValue::CFunctionType func) {
    return heap.alloc_function(func);
}
BoxType* Interpreter::alloc_box(TackValue val) {
    return heap.alloc_box(val);
}
TackValue::StringType* Interpreter::intern_string(const std::string& data) {
    auto got = key_cache.find(data);
    if (got == key_cache.end()) {
        auto put = key_cache.put(data);
        auto str = new TackValue::StringType { data };
        key_cache.value_at(put) = str;
        return str;
    }
    return key_cache.value_at(got);
}
TackValue::StringType* Interpreter::alloc_string(const std::string& data) {
    return heap.alloc_string(data);
}

TackValue Interpreter::get_global(const std::string& name) {
    return get_global(name, GLOBAL_NAMESPACE);
}
TackValue Interpreter::get_global(const std::string& name, const std::string& module_name) {
    auto mod = modules.find(module_name);
    if (mod == modules.end()) {
        return TackValue::null();
    }
    auto* module = modules.value_at(mod);

    auto* var = module->lookup(name);
    if (!var) {
        return TackValue::null();
    }

    return globals[var->g_id];
}
TackValue Interpreter::get_type_name(TackType type) {
    static auto nulltype    = TackValue::string(intern_string("null"));
    static auto booltype    = TackValue::string(intern_string("boolean"));
    static auto numtype     = TackValue::string(intern_string("number"));
    static auto stringtype  = TackValue::string(intern_string("string"));
    static auto ptrtype     = TackValue::string(intern_string("pointer"));
    static auto objecttype  = TackValue::string(intern_string("object"));
    static auto arraytype   = TackValue::string(intern_string("array"));
    // static auto cftype      = TackValue::string(intern_string("cfunction"));
    static auto ftype       = TackValue::string(intern_string("function"));
    static auto unknown     = TackValue::string(intern_string("unknown"));

    switch (type) {
        case TackType::Boolean: return booltype;
        case TackType::Number:  return numtype;
        case TackType::Null:    return nulltype;
        case TackType::String:  return stringtype;
        case TackType::Pointer: return ptrtype;
        case TackType::Object:  return objecttype;
        case TackType::Array:   return arraytype;
        // case TackType::CFunction:return cftype;
        case TackType::Function:return ftype;
        default: return unknown;
    }
}

Compiler::VariableContext* Interpreter::set_global_v(const std::string& name, TackValue value, bool is_const) {
    return set_global_v(name, GLOBAL_NAMESPACE, value, is_const);
}

Compiler::VariableContext* Interpreter::set_global_v(const std::string& name, const std::string& module_name, TackValue value, bool is_const) {
    auto iter = modules.find(module_name);
    if (iter == modules.end()) {
        error("Error: module not found:" + module_name);
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

Compiler::ScopeContext* Interpreter::load_module_s(const std::string& module_name) {
    auto iter = modules.find(module_name);
    if (iter == modules.end()) {
        auto filename = module_name;
        auto file_data = read_text_file(filename);
        if (!file_data.has_value()) {
            for (auto& d : module_dirs) {
                auto path = std::filesystem::path(d).append(filename).string();
                file_data = read_text_file(path);
                if (file_data.has_value()) {
                    break;
                }
            }
            if (!file_data.has_value()) {
                error("Unable to load module: " + module_name);
                return nullptr;
            }
        }
    
        // parse
        auto out_ast = AstNode {};
        parse(file_data.value(), out_ast);
        auto ast = AstNode(AstType::FuncLiteral, AstNode(AstType::ParamDef), out_ast);
        
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
        call(TackValue::function(func), 0, nullptr);
        func->refcount -= 1;

        return scope;

    } else {
        // already loaded
        // log("Module already loaded: ", module_name, " - already loaded");
        return modules.value_at(iter);
    }
    
}


void Interpreter::error(const std::string& msg) {
    /*
    * // TODO: dump stack
    auto ln = _pr->bytecode->line_numbers[_pc];
    std::cout << "encountered at line: " << ln << " in " << _pr->bytecode->name << std::endl;
    auto s = stackbase;
    while (stack[s - 1]._i != 0) {
        auto retpc = stack[s - 3]._i;
        auto func = ((TackValue::FunctionType*)stack[s - 2]._p)->bytecode;
        auto retln = func->line_numbers[retpc];
        log(" - called from line:", retln, "in", func->name);
        s = stack[s - 1]._i;
    }*/
    throw std::runtime_error(msg);
}

#define handle(opcode)  break; case Opcode::opcode:
#define REGISTER_RAW(n) stack[stackbase+n]
#define REGISTER(n)     (*(value_is_boxed(REGISTER_RAW(n)) ? &value_to_boxed(REGISTER_RAW(n))->value : &REGISTER_RAW(n)))
#define check(v, ty)    if (!(v).is_##ty()) error("type error: expected " #ty);
#define in_error(msg)   error(msg + ((CodeFragment*)_pr->code_ptr)->name + std::to_string(((CodeFragment*)_pr->code_ptr)->line_numbers[_pc]))

TackValue Interpreter::call(TackValue fn, int nargs, TackValue* args) {
    if (!fn.is_function()) {
        error("type error: call() expects function type");
    }
    
    auto* _pr = fn.function();
    if (_pr->is_cfunction) {
        // error("Interpreter::call() with cfunction");
        return ((*((TackValue::CFunctionType*)_pr->code_ptr))(this, nargs, args));
    }
    
    // it's a tack-defined function not a cfunction
    auto _pc = 0u; // program counter
    auto _pe = ((CodeFragment*)_pr->code_ptr)->instructions.size();

    // stack/registers
    auto initial_stackbase = stackbase;
    stackbase += STACK_FRAME_OVERHEAD;

    // copy arguments to stack
    if (nargs && args != &stack[stackbase]) {
        std::memcpy(&stack[stackbase], args, sizeof(TackValue) * nargs);
    }

    // set up initial call frame
    REGISTER_RAW(-3)._i = ((CodeFragment*)_pr->code_ptr)->instructions.size();
    REGISTER_RAW(-2)._p = (void*)_pr;
    REGISTER_RAW(-1)._i = initial_stackbase; // special case

    while (_pc < _pe) {
        auto i = ((CodeFragment*)_pr->code_ptr)->instructions[_pc];
        switch (i.opcode) {
        case Opcode::UNKNOWN: break;
            handle(ZERO_CAPTURE) {
                REGISTER_RAW(i.r0) = TackValue::null();
            }
            handle(LOAD_I_SN) {
                REGISTER(i.r0) = TackValue::number(i.u1);
            }
            handle(LOAD_I_NULL) {
                REGISTER(i.r0) = TackValue::null();
            }
            handle(LOAD_I_BOOL) {
                REGISTER(i.r0) = TackValue::boolean(i.u8.r1);
            }
            handle(LOAD_CONST) {
                REGISTER(i.r0) = ((CodeFragment*)_pr->code_ptr)->storage[i.u1];
            }
            handle(INCREMENT) {
                auto r0 = REGISTER(i.r0);
                check(r0, number);
                REGISTER(i.r0) = TackValue::number(REGISTER(i.r0).number() + 1);
            }
            handle(ADD) {
                auto lhs = REGISTER(i.u8.r1);
                auto rhs = REGISTER(i.u8.r2);
                auto lt = lhs.get_type();
                if (lt == TackType::Number) {
                    // numeric add
                    check(rhs, number);
                    REGISTER(i.r0) = TackValue::number(lhs.number() + rhs.number());
                } else if (lt == TackType::String) {
                    // string add
                    check(rhs, string);
                    auto l = lhs.string();
                    auto r = rhs.string();
                    auto new_str = std::string(l->data) + std::string(r->data);
                    REGISTER(i.r0) = TackValue::string(alloc_string(new_str));
                } else if (lt == TackType::Array) {
                    // array add
                    check(rhs, array);
                    auto l = lhs.array();
                    auto r = rhs.array();
                    auto n = alloc_array();
                    std::copy(l->data.begin(), l->data.end(), std::back_inserter(n->data));
                    std::copy(r->data.begin(), r->data.end(), std::back_inserter(n->data));
                    REGISTER(i.r0) = TackValue::array(n);
                } else {
                    in_error("operator '+' expected number / array / string");
                }
            }
            handle(SUB) {
                auto lhs = REGISTER(i.u8.r1);
                auto rhs = REGISTER(i.u8.r2);
                check(lhs, number);
                check(rhs, number);
                REGISTER(i.r0) = TackValue::number(lhs.number() - rhs.number());
            }
            handle(MUL) {
                auto lhs = REGISTER(i.u8.r1);
                auto rhs = REGISTER(i.u8.r2);
                check(lhs, number);
                check(rhs, number);
                REGISTER(i.r0) = TackValue::number(lhs.number() * rhs.number());
            }
            handle(DIV) {
                auto lhs = REGISTER(i.u8.r1);
                auto rhs = REGISTER(i.u8.r2);
                check(lhs, number);
                check(rhs, number);
                REGISTER(i.r0) = TackValue::number(lhs.number() / rhs.number());
            }
            handle(MOD) {
                auto lhs = REGISTER(i.u8.r1);
                auto rhs = REGISTER(i.u8.r2);
                check(lhs, number);
                check(rhs, number);
                REGISTER(i.r0) = TackValue::number(fmod(lhs.number(), rhs.number()));
            }
            handle(POW) {
                auto lhs = REGISTER(i.u8.r1);
                auto rhs = REGISTER(i.u8.r2);
                check(lhs, number);
                check(rhs, number);
                REGISTER(i.r0) = TackValue::number(pow(lhs.number(), rhs.number()));
            }
                
            handle(SHL) {
                auto lhs = REGISTER(i.u8.r1);
                auto rhs = REGISTER(i.u8.r2);
                if (lhs.is_array()) {
                    auto* arr = lhs.array();
                    arr->data.emplace_back(rhs);
                    // put the appended value into r0
                    REGISTER(i.r0) = rhs;
                } else if (lhs.is_number()) {
                    check(rhs, number);
                    REGISTER(i.r0) = TackValue::number(
                        (uint32_t)lhs.number() << 
                        (uint32_t)rhs.number()
                    );
                } else {
                    in_error("expected number or array");
                }
            }
            handle(SHR) {
                auto lhs = REGISTER(i.u8.r1);
                auto rhs = REGISTER(i.u8.r2);
                if (lhs.is_array()) {
                    auto* arr = lhs.array();
                    // put the popped value into r0
                    REGISTER(i.r0) = arr->data.back();
                    arr->data.pop_back();
                } else if (lhs.is_number()) {
                    check(rhs, number);
                    REGISTER(i.r0) = TackValue::number(
                        (uint32_t)lhs.number() >>
                        (uint32_t)rhs.number()
                    );
                } else {
                    in_error("expected number or array");
                }
            }
                
            handle(AND) {
                REGISTER(i.r0) = TackValue::boolean(REGISTER(i.u8.r1).get_truthy() && REGISTER(i.u8.r2).get_truthy());
            }
            handle(OR) {
                REGISTER(i.r0) = TackValue::boolean(REGISTER(i.u8.r1).get_truthy() || REGISTER(i.u8.r2).get_truthy());
            }
            handle(IN) {
                auto test_val = REGISTER(i.u8.r1);
                auto arr_val = REGISTER(i.u8.r2);
                if (arr_val.is_array()) {
                    auto* arr = arr_val.array();
                    auto found = TackValue::false_();
                    for (auto i : arr->data) {
                        if (i == test_val) {
                            found = TackValue::true_();
                            break;
                        }
                    }
                    REGISTER(i.r0) = found;
                } else if (arr_val.is_object()) {
                    check(test_val, string);
                    auto* obj = arr_val.object();
                    auto* str = test_val.string();
                    auto f = obj->data.find(str->data);
                    REGISTER(i.r0) = (f == obj->data.end()) ? TackValue::false_() : TackValue::true_();
                } else {
                    in_error("in: expected array or object");
                }
            }
                
            handle(EQUAL) {
                REGISTER(i.r0) = TackValue::boolean(REGISTER(i.u8.r1) == REGISTER(i.u8.r2));
            }
            handle(NEQUAL) {
                REGISTER(i.r0) = TackValue::boolean(!(REGISTER(i.u8.r1) == REGISTER(i.u8.r2)));
            }
            handle(LESS) {
                auto lhs = REGISTER(i.u8.r1);
                auto rhs = REGISTER(i.u8.r2);
                check(lhs, number);
                check(rhs, number);
                REGISTER(i.r0) = TackValue::boolean(lhs.number() < rhs.number());
            }
            handle(LESSEQ) {
                auto lhs = REGISTER(i.u8.r1);
                auto rhs = REGISTER(i.u8.r2);
                check(lhs, number);
                check(rhs, number);
                REGISTER(i.r0) = TackValue::boolean(lhs.number() <= rhs.number());
            }
            handle(GREATER) {
                auto lhs = REGISTER(i.u8.r1);
                auto rhs = REGISTER(i.u8.r2);
                check(lhs, number);
                check(rhs, number);
                REGISTER(i.r0) = TackValue::boolean(lhs.number() > rhs.number());
            }
            handle(GREATEREQ) {
                auto lhs = REGISTER(i.u8.r1);
                auto rhs = REGISTER(i.u8.r2);
                check(lhs, number);
                check(rhs, number);
                REGISTER(i.r0) = TackValue::boolean(lhs.number() >= rhs.number());
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
                auto var = REGISTER(i.r0);
                auto end = REGISTER(i.u8.r1);
                check(var, number);
                check(end, number);
                if (var.number() < end.number()) {
                    _pc++;
                }
            }
            handle(FOR_ITER_INIT) {
                auto iter_val = REGISTER(i.u8.r1);
                auto iter_type = iter_val.get_type();
                if (iter_type == TackType::Array) {
                    REGISTER_RAW(i.r0)._i = 0;
                } else if (iter_type == TackType::Object) {
                    REGISTER_RAW(i.r0)._i = iter_val.object()->data.begin();
                // } else if (iter_type == Type::Function) {
                } else {
                    in_error("for loop expected array or object");
                }
            }
            handle(FOR_ITER) {
                auto iter_val = REGISTER(i.u8.r1);
                auto iter_type = iter_val.get_type();
                if (iter_type == TackType::Array) {
                    auto ind = REGISTER_RAW(i.r0)._i;
                    auto arr = iter_val.array();
                    if (ind < arr->data.size()) {
                        REGISTER(i.u8.r2) = arr->data.at(ind);
                        _pc++;
                    }
                } else if (iter_type == TackType::Object) {
                    auto it = REGISTER_RAW(i.r0)._i;
                    auto obj = iter_val.object();
                    if (it != obj->data.end()) {
                        auto& key = obj->data.key_at(it);
                        auto cached = intern_string(key);
                        REGISTER(i.u8.r2) = TackValue::string(cached);
                        _pc++;
                    }
                // } else if (iter_type == Type::Function) {
                }
            }
            handle(FOR_ITER2) {
                auto iter_val = REGISTER(i.u8.r1);
                check(iter_val, object);
                auto obj = iter_val.object();
                auto it = REGISTER_RAW(i.r0)._i;
                if (it != obj->data.end()) {
                    auto& key = obj->data.key_at(it);
                    auto cached = intern_string(key);
                    REGISTER(i.u8.r2) = TackValue::string(cached);
                    REGISTER(i.u8.r2 + 1) = obj->data.value_at(it);
                    _pc++;
                }
            }
            handle(FOR_ITER_NEXT) {
                auto iter_val = REGISTER(i.u8.r1);
                auto iter_type = iter_val.get_type();
                if (iter_type == TackType::Array) {
                    // increment index
                    REGISTER_RAW(i.r0)._i += 1;
                } else if (iter_type == TackType::Object) {
                    // next key
                    auto obj = iter_val.object();
                    REGISTER_RAW(i.r0)._i = obj->data.next(REGISTER_RAW(i.r0)._i);
                }
            }
            handle(CONDSKIP) {
                if (REGISTER(i.r0).get_truthy()) {
                    _pc++;
                }
            }
            handle(JUMPF) { _pc += i.u1 - 1; }
            handle(JUMPB) { _pc -= i.u1 + 1; }
            handle(LEN) {
                auto val = REGISTER(i.u8.r1);
                auto type = val.get_type();
                if (type == TackType::Array) {
                    REGISTER(i.r0) = TackValue::number(val.array()->data.size());
                } else if (type == TackType::String) {
                    REGISTER(i.r0) = TackValue::number(val.string()->data.size());
                } else if (type == TackType::Object) {
                    REGISTER(i.r0) = TackValue::number(val.object()->data.size());
                } else {
                    in_error("operator '#' expected string / array / object");
                }
            }
            handle(NEGATE) {
                auto val = REGISTER(i.u8.r1);
                check(val, number);
                REGISTER(i.r0) = TackValue::number(-(val.number()));
            }
            handle(NOT) {
                auto val = REGISTER(i.u8.r1);
                REGISTER(i.r0) = TackValue::boolean(!val.get_truthy());
            }
            handle(READ_CAPTURE) {
                REGISTER_RAW(i.r0) = _pr->captures[i.u8.r1];
            }
            handle(ALLOC_FUNC) {
                // create closure
                auto code = (CodeFragment*)((CodeFragment*)_pr->code_ptr)->storage[i.u1].pointer(); // assumed correct type due to compiler
                auto* func = heap.alloc_function(code);
                // capture captures; box inplace if necessary
                for (const auto& c : code->capture_info) {
                    auto val = REGISTER_RAW(c.source_register);
                    if (!value_is_boxed(val)) {
                        val = value_from_boxed(heap.alloc_box(val));
                        REGISTER_RAW(c.source_register) = val;
                    }
                    func->captures.emplace_back(val);
                }
                // done
                REGISTER(i.r0) = TackValue::function(func);
            }
            handle(ALLOC_ARRAY) {
                auto* arr = heap.alloc_array();
                // emplace child elements
                for (auto e = 0; e < i.u8.r1; e++) {
                    arr->data.emplace_back(REGISTER(i.u8.r2 + e));
                }
                REGISTER(i.r0) = TackValue::array(arr);
            }
            handle(ALLOC_OBJECT) {
                auto* obj = heap.alloc_object();
                // emplace child elements
                for (auto e = 0; e < i.u8.r1; e++) {
                    auto key = REGISTER(i.u8.r2 + e * 2).string(); // assumed correct type due to compiler
                    auto val = REGISTER(i.u8.r2 + e * 2 + 1);
                    obj->data.set(key->data, val);
                }
                REGISTER(i.r0) = TackValue::object(obj);
            }
            handle(LOAD_ARRAY) {
                auto arr_val = REGISTER(i.u8.r1);
                auto ind_val = REGISTER(i.u8.r2);

                if (arr_val.is_array()) {
                    check(ind_val, number);
                    auto* arr = arr_val.array();
                    auto ind = ind_val.number();
                    if (ind >= arr->data.size() || ind < 0) {
                        in_error("index out of range");
                    }
                    REGISTER(i.r0) = arr->data.at(ind);
                } else if (arr_val.is_object()) {
                    check(ind_val, string);
                    auto* obj = arr_val.object();
                    auto* str = ind_val.string();
                    auto f = obj->data.find(str->data);
                    if (f == obj->data.end()) {
                        in_error("key not found: " + ind_val.get_string());
                    }
                    REGISTER(i.r0) = obj->data.value_at(f);
                } else {
                    in_error("[]: expected array or object");
                }
            }
            handle(STORE_ARRAY) {
                auto arr_val = REGISTER(i.u8.r1);
                auto ind_val = REGISTER(i.u8.r2);

                if (arr_val.is_array()) {
                    check(ind_val, number);
                    auto* arr = arr_val.array();
                    auto ind = ind_val.number();
                    if (ind > arr->data.size()) {
                        in_error("index out of range");
                    }
                    arr->data.at(ind) = REGISTER(i.r0);
                } else if (arr_val.is_object()) {
                    check(ind_val, string);
                    auto* obj = arr_val.object();
                    auto* str = ind_val.string();
                    obj->data.value_at(obj->data.put(str->data)) = REGISTER(i.r0);
                } else {
                    in_error("[]: expected array or object");
                }
            }
            handle(LOAD_OBJECT) {
                auto lhs = REGISTER(i.u8.r1);
                auto rhs = REGISTER(i.u8.r2);
                check(rhs, string);
                auto key = rhs.string();

                if (lhs.is_number()) {
                    // auto code = nullptr; // look up function from vtable for number
                    // auto* func = heap.alloc_function(code);
                    // auto self = REGISTER_RAW(i.u8.r1);
                    // if (!value_is_boxed(self)) {
                    //     auto box = value_from_boxed(heap.alloc_box(self));
                    //     func->self = box;
                    // }
                    // REGISTER(i.r0) = func;

                    /*
                    Several pre-requisites here:
                        - Vtables: Fixed for the fixed types
                        then we will add custom ones later

                        - Cfunction needs to be able to accept captures
                        or at least a single 'self' capture
                        (for the builtins, type can be safely assumed, but need to check boxing still)

                        - Try and elide the allocation.
                        Detect immediate self-calls like this in the compiler
                        and emit a custom LOAD_CALL opcode
                            that combines LOAD_CALL and CALL

                        LOAD_CALL would also handle this case
                            let a = 1
                            let b = { foo = fn() {} }

                            a.to_string()
                            b.foo()

                        b.foo() will also compile to LOAD_CALL, but it can detect that foo
                        comes from the object not the vtable, so it won't do the binding
                        (or maybe it doesn't matter)

                        - Vtable methods MUST NOT allow self to escape
                        (eg by returning another bound method)
                    */


                } else if (lhs.is_object()) {
                    auto* obj = lhs.object();
                    auto found = false;
                    auto val = obj->data.get(key->data, found);
                    if (!found) {
                        in_error("key not found: "s + key->data);
                    } else {
                        REGISTER(i.r0) = val;
                    }
                } else {
                    in_error("unknown type for LOAD_OBJECT");
                }
            }
            handle(STORE_OBJECT) {
                auto lhs = REGISTER(i.u8.r1);
                auto key_val = REGISTER(i.u8.r2);
                check(lhs, object);
                check(key_val, string);
                auto* obj = lhs.object();
                auto key = key_val.string();
                obj->data.set(key->data, REGISTER(i.r0));
            }
            handle(CALL) {
                auto r0 = REGISTER(i.r0);
                if (r0.is_function()) {
                    auto func = r0.function();
                    if (func->is_cfunction) {
                        auto cfunc = (TackValue::CFunctionType*)func->code_ptr;
                        auto nargs = i.u8.r1;
                            
                        auto old_base = stackbase;
                        stackbase = stackbase + i.u8.r2 + STACK_FRAME_OVERHEAD;                        
                        auto retval = (*cfunc)(this, nargs, &stack[stackbase]);
                        stackbase = old_base;
                        REGISTER_RAW(i.u8.r2) = retval;
                    } else {
                        auto bytecode = (CodeFragment*)func->code_ptr;
                        auto nargs = i.u8.r1;
                        auto correct_nargs = 0; // func->bytecode->nargs;
                        // TODO: arity checking
                        if (false && nargs != correct_nargs) {
                            in_error("wrong nargs");
                        }
                        auto new_base = i.u8.r2 + STACK_FRAME_OVERHEAD;

                        if (new_base + bytecode->max_register > MAX_STACK) {
                            in_error("would exceed stack");
                        }

                        // set up call frame
                        REGISTER_RAW(new_base - 3)._i = _pc; // push return addr
                        REGISTER_RAW(new_base - 2)._p = (void*)_pr; // return program
                        REGISTER_RAW(new_base - 1)._i = stackbase; // push return frameptr

                        _pr = func;
                        _pe = bytecode->instructions.size();
                        _pc = -1;
                        stackbase = stackbase + new_base; // new stack frame
                    }
                } else {
                    in_error("tried to call non-function");
                }
            }
            handle(RET) {
                auto return_addr = REGISTER_RAW(-3);
                auto return_func = REGISTER_RAW(-2);
                auto return_stack = REGISTER_RAW(-1);
                auto return_val = REGISTER(i.u8.r1);

                _pc = return_addr._i;
                _pr = (TackValue::FunctionType*)return_func._p;
                _pe = ((CodeFragment*)_pr->code_ptr)->instructions.size();

                // "Clean" the stack - Must not leave any boxes in unused registers or subsequent loads to register will mistakenly write-through
                std::memset(stack.data() + stackbase, 0xffffffff, MAX_REGISTERS * sizeof(TackValue));
                    
                REGISTER_RAW(-3) = return_val;
                REGISTER_RAW(-2) = TackValue::null();
                REGISTER_RAW(-1) = TackValue::null();
                heap.gc(globals, stack, stackbase);
                    
                stackbase = return_stack._i;
                if (stackbase == initial_stackbase) {
                    return return_val;
                }
            }
        break; default: in_error("unknown instruction: " + to_string(i.opcode));
        }
        _pc++;
    }
    error("Reached end of interpreter loop - should be impossible");
    return TackValue::null(); // should be unreachable
}

#undef check
#undef handle
#undef REGISTER
#undef REGISTER_RAW
