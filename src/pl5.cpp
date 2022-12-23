#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <string_view>
#include <array>
#include <unordered_map>
#include <chrono>

#define JLIB_IMPLEMENTATION
#include <jlib/log.h>
#include <jlib/text_file.h>
#include <jlib/test_framework.h>

#include "parsing.h"

using namespace std;
using namespace std::chrono;

/*
Grammar:

	module
		stat-list
	
	stat-list
		stat stat-list

	stat
		if-stat
		for-in-stat
		var-decl
		assign-stat
		exp

	if-stat
		if-block
		if-block else-block
		
		if-block
			'if' exp block
		else-block
			'else' if-stat // chained else-if here
			'else' block   // terminating else

	for-in-stat
		'for' ident 'in' exp block
	
	block
		'{' stat-list '}'
	
	var-decl
		'let' ident '=' exp

	assign-stat
		locator '=' exp

	locator
		access-exp
		index-exp
		primary-exp

	exp:
		ternary-exp
			binary-exp '?' ternary-exp ':' ternary-exp
			binary-exp

	binary-exp
			or-exp		and-exp [ || or-exp ]			// left-assoc because of short circuiting
			and-exp		bit-or-exp [ && and-exp ]		// left-assoc because of short circuiting

			bit-or-exp	bit-xor-exp [ | bit-or-exp ]	// left assoc
			bit-xor-exp	bit-and-exp [ ^ bit-xor-exp ]	// associative
			bit-and-exp cmp-exp [ & bit-and-exp ]		// associative

			cmp-exp		shl-exp [ cmp-op cmp-exp ]		// TODO: don't allow chained comparisons a == b == c

			shl-exp		shr-exp [ << shl-exp ]			// left associative
			shr-exp		add-exp [ >> adr-exp ]			// left associative

			add-exp		sub-exp [ + add-exp ]			// associative
			sub-exp		mul-exp	[ - sub-exp ]			// left associative
			mul-exp		div-exp [ * mul-exp ]			// associative
			div-exp		mod-exp [ / div-exp ]			// left associative
			mod-exp		pow-exp [ % mod-exp ]			// left associative
			pow-exp		unary-exp [ ** pow-exp ]		// left associative
		
		unary-exp
			'-' unary-exp
			'!' unary-exp
			'~' unary-exp
			postfix-exp

		postfix-exp
			call-exp
			access-exp
			index-exp
			primary-exp

			call-exp
				postfix-exp '(' args-list ')'

			access-exp
				postfix-exp '.' identifier
		
			index-exp
				postfix-exp '[' exp ']'

		primary-exp
			literal
			identifier
			'(' exp ')'

		literal
			'null'
			'true'
			'false'
			literal-number
			literal-string
			literal-object
			literal-array
			literal-function
			

Types:
	scalarvalue types
		null:		null
		bool:		true, false
		int:		64 bit integer
		pointer:	32 or 64 bit opaque pointer for c/c++/(rust?) interop
		double		64 bit float
		string		utf8 string
					can do short string optimization
					can do interning for longer strings
		vector      2x or 3x or 4x
		matrix		{ 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }

	ref types
		array		[ 0, 1, 2, ... ]
		object		{ a = 1, b = 2, ... }
		function	(x, y, z) {} // closures are passed by-ref as well

Notes:
	objects will need some kind of 'prototype' or 'index' field to implement classes

	array is not just object in disguise, it can be implemented completely differently (fast indexing, no holes, etc)

	might do some operator overloading python or lua style

	function value could implement with pointer-to-code and pointer-to-closure
	for a class function we could use the "self" object as the closure
	for a free function the closure is the containing scope
	captured scopes will need to capture parent scopes as well (up to module or function boundary)
	so will need a linked list or something

	some syntax sugar:
		let f = (x, y) {}
		def f(x, y) {}

	this is assumed to be running as part of a game engine
	so make sure the garbage collector can be done in parallel slices multi threaded and deterministic time limits

	also make it possible to jit some of the hot functions / hot loops
	(once we have a bytecode interpreter)

*/

/*
Notes:
	bits of double: 1 sign 11 exp 52 frac
	nan: exp==1, frac!=0
		we will set the top 16 bits to 1
		and use the bottom 48 for whatever
	inf: exp==1, frac==0
	zero:exp==0, frac==0
	nan happens when exp is all 1, frac is non-zero (sign bit ignored)
	quick and safe to set first 16 bits to 1, and use the remaining 48 bits for whatever
*/

enum class Type {
	Null,
	Boolean,
	Integer,
	Number,
	String,

	//Pointer,
	//Vector,
	//Matrix,
	Array,
	Object,
	Function
};

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


// tree walking reference interpreter
struct RefInterpreter {
	//struct HeapValue {
	//	virtual ~HeapValue() {}
	//};
	//struct Object : HeapValue {
	//	unordered_map<string, double> values;
	//};
	//struct Array : HeapValue {
	//	vector<double> values;
	//};
	//struct Function : HeapValue {
	//	const AstNode* code; // the code
	//};
	//
	//// nan tagging for references
	//vector<HeapValue*> heap;
	//vector<Object*> stack; // one object per stack frame, they are stored on the heap
	/*

	mark and sweep gc:
		gc_roots = everything that can be reached from current call stack
			local variables in stack frames for call stack
			global variables (~= local variables in stack frame 0)
			any frames closed over by frames in current call stack
			... needs thought

		// mark
		VISITED = [ false for x in heap ]
		def visit(node) {
			VISITED[node] = true

			if node is array
				for x in node
					visit(x)
		}

		// sweep
		for x in heap
			if not VISITED(x)
				deallocate(x)

	*/

	vector<unordered_map<string, double>> scopes;
	vector<AstNode> functions; // interned functions
	vector<string> strings; // interned string literals

	RefInterpreter() {
		scopes.emplace_back(); // global variables
	}

	void push_scope(auto x) {
		scopes.emplace_back(x);
	}
	void pop_scope() {
		scopes.pop_back();
	}

	template<bool should_exist = true>
	double& lookup(const string& name) {
		auto& scope = scopes.back();
		auto var = scopes.back().find(name);
		if constexpr (should_exist) {
			if (var == scope.end()) {
				throw runtime_error("Variable not found: " + name);
			}
			return var->second;
		} else {
			if (var != scope.end()) {
				// throw runtime_error("Variable already existed: " + name);
			}
			return scope[name];
		}
	}

	double eval(const AstNode& ast) {
		switch (ast.type) {
			case AstType::StatList: {
				for (auto& c : ast.children) {
					auto val = eval(c);
					if (c.type == AstType::ReturnStat) {
						return val;
					}
				}
				return 0;
			}
			case AstType::VarDeclStat: {
				lookup<false>(ast.children[0].data_s) = eval(ast.children[1]);
				return 0;
			}
			case AstType::AssignStat: {
				// evaluate the lhs to locate the storage for the new value
				// this is a bit messy: lhs must be a AccessExp, IndexExp, or Identifier, anything else is an error
				// if lhs is an AccessExp then lhs is { lhs-lhs, identifier }; lhs-lhs must evaluate to an object
				// if lhs is an IndexExp then lhs is { lhs-lhs, exp }; lhs-lhs must evaluate to an array
				// if lhs is an identifier, it's either in scope or captured

				// you can't have the lhs be a function call or any other expression until we have proper value categories
				// which enables to return some kind of 'locator-value' (like lvalues) from functions (and maybe expressions if we allow them to be overridden)

				auto& lhs = ast.children[0];
				if (lhs.type == AstType::IndexExp) {
					/*
					locator = eval(lhs.children[0]);
					if locator.type != Array {
						error "Not an array"
					index = eval(lhs.children[1])
					rhs = eval(ast.children[1])
					locator[index].set(rhs)
					*/
				}
				else if (lhs.type == AstType::AccessExp) {
					/*
					locator = eval(lhs.children[0]);
					if locator.type != Object
						error "Not an object"
					rhs = eval(ast.children[1])
					ident = lhs.children[1];
					locator[ident.data_s].set(rhs)
					*/
				}
				else if (lhs.type == AstType::Identifier) {
					lookup(ast.children[0].data_s) = eval(ast.children[1]);
				}
				else {
					throw runtime_error("LHS of an assignment must be an identifier, access expression, or index expression");
				}

				return 0;
			}
			case AstType::PrintStat: {
				log<true, false>("(print)", eval(ast.children[0]));
				return 0;
			}
			case AstType::IfStat: {
				if (eval(ast.children[0])) {
					eval(ast.children[1]);
				} else {
					if (ast.children.size() > 2) {
						eval(ast.children[2]);
					}
				}
				return 0;
			}
			case AstType::WhileStat: {
				while (eval(ast.children[0])) {
					eval(ast.children[1]);
				}
				return 0;
			}
			case AstType::ReturnStat: {
				if (ast.children.size()) {
					return eval(ast.children[0]);
				}
				return 0.0;
			}

			case AstType::TernaryExp: return eval(ast.children[0]) ? eval(ast.children[1]) : eval(ast.children[2]);
#define EVAL_BIN(type, op)\
			case AstType::type: return eval(ast.children[0]) op eval(ast.children[1]);
#define EVAL_BIN_C(type, c, op)\
			case AstType::type: return (c)eval(ast.children[0]) op (c)eval(ast.children[1]);
#define EVAL_BIN_F(type, expr)\
			case AstType::type: return expr(eval(ast.children[0]), eval(ast.children[1]));
			EVAL_BIN(OrExp, ||)
			EVAL_BIN(AndExp, &&)
			EVAL_BIN_C(BitOrExp, uint64_t, |)
			EVAL_BIN_C(BitAndExp, uint64_t, &)
			EVAL_BIN_C(BitXorExp, uint64_t, ^)
			EVAL_BIN(EqExp, ==)
			EVAL_BIN(NotEqExp, !=)
			EVAL_BIN(LessExp, <)
			EVAL_BIN(GreaterExp, >)
			EVAL_BIN(LessEqExp, <=)
			EVAL_BIN(GreaterEqExp, >=)
			EVAL_BIN_C(ShiftLeftExp, uint64_t, <<)
			EVAL_BIN_C(ShiftRightExp, uint64_t, >>)
			EVAL_BIN(AddExp, +);
			EVAL_BIN(SubExp, -);
			EVAL_BIN(MulExp, *);
			EVAL_BIN(DivExp, / );
			EVAL_BIN_C(ModExp, uint64_t, %);
			EVAL_BIN_F(PowExp, pow);

			case AstType::NegateExp: return -eval(ast.children[0]);
			case AstType::NotExp: return !eval(ast.children[0]);
			case AstType::BitNotExp: return ~(uint64_t)eval(ast.children[0]);

			case AstType::CallExp: {
				auto func = eval(ast.children[0]);
				if (!isnan(func)) {
					throw runtime_error("Tried to call a non-function");
				}
				auto& fn = functions[nan_unpack_int(func)];

				// evaluate arguments
				auto s = unordered_map<string, double> {};

				// must be same arguments
				auto& param_def = fn.children[0];
				auto& arg_list = ast.children[1];
				if (param_def.children.size() != ast.children.size()) {
					throw runtime_error("Tried to call function with " + to_string(ast.children.size()) + "args, expected" + to_string(param_def.children.size()));
				}
				for (auto i = 0; i < param_def.children.size(); i++) {
					s.insert({ param_def.children[i].data_s, eval(arg_list.children[i]) });
				}
				push_scope(s);
				
				// evaluate function
				auto retval = eval(fn.children[1]);
				pop_scope();
				return retval;
			};
			// case AstType::ArgList: { log("ArgList should be unreachable"); return 0; }
			case AstType::IndexExp: { return 0; }
			case AstType::AccessExp: { return 0; }

			case AstType::FuncLiteral: {
				functions.emplace_back(ast);
				return nan_pack_int(functions.size() - 1);
			};
			// case AstType::ParamDef: { log("ParamDef should be unreachable!");  return 0; }
			case AstType::StringLiteral: {
				strings.emplace_back(ast.data_s);
				return nan_pack_int(strings.size() - 1);
			}
			case AstType::NumLiteral: { return ast.data_d; }
			case AstType::Identifier: { return lookup(ast.data_s); }

			case AstType::Unknown:default: log("WARNING: unknown AST"); return 0;
		}
	}
};

// Instruction is a list of uint64_t
// Each instruction has up to 3 arguments

// CONDJUMP r0 i0 i1 
// - if r0 is true, jump to i0, otherwise jump to i1
// TODO: condjump i0 and i1 should be more than 16bit
// can mitigate by making it a relative jump (?) but still problematic

#define opcodes() \
	opcode(UNKNOWN)\
	opcode(EQUAL) opcode(NEQUAL) opcode(GREATER) opcode(LESS) opcode(GREATEREQ) opcode(LESSEQ)\
	opcode(ADD) opcode(SUB) opcode(DIV) opcode(MUL) opcode(MOD) opcode(POW)\
	opcode(NEGATE) opcode(NOT) opcode(BITNOT)\
	opcode(LOAD) opcode(STORE)\
	opcode(JUMP) opcode(CONDJUMP)\
	opcode(CALL) opcode(PRINT)
	

#define opcode(x) x,
enum class Opcode : uint8_t { opcodes() };
#define opcode(x) { Opcode::x, #x },
unordered_map<Opcode, string> opcode_to_string = { opcodes() };
#undef opcode

#ifdef _WIN32
#include <malloc.h>
#endif

/**
 * Allocator for aligned data.
 *
 * Modified from the Mallocator from Stephan T. Lavavej.
 * <http://blogs.msdn.com/b/vcblog/archive/2008/08/28/the-mallocator.aspx>
 */
template <typename T, std::size_t Alignment>
class aligned_allocator
{
public:

	// The following will be the same for virtually all allocators.
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;
	typedef std::size_t size_type;
	typedef ptrdiff_t difference_type;

	T* address(T& r) const
	{
		return &r;
	}

	const T* address(const T& s) const
	{
		return &s;
	}

	std::size_t max_size() const
	{
		// The following has been carefully written to be independent of
		// the definition of size_t and to avoid signed/unsigned warnings.
		return (static_cast<std::size_t>(0) - static_cast<std::size_t>(1)) / sizeof(T);
	}


	// The following must be the same for all allocators.
	template <typename U>
	struct rebind
	{
		typedef aligned_allocator<U, Alignment> other;
	};

	bool operator!=(const aligned_allocator& other) const
	{
		return !(*this == other);
	}

	void construct(T* const p, const T& t) const
	{
		void* const pv = static_cast<void*>(p);

		new (pv) T(t);
	}

	void destroy(T* const p) const
	{
		p->~T();
	}

	// Returns true if and only if storage allocated from *this
	// can be deallocated from other, and vice versa.
	// Always returns true for stateless allocators.
	bool operator==(const aligned_allocator& other) const
	{
		return true;
	}


	// Default constructor, copy constructor, rebinding constructor, and destructor.
	// Empty for stateless allocators.
	aligned_allocator() { }

	aligned_allocator(const aligned_allocator&) { }

	template <typename U> aligned_allocator(const aligned_allocator<U, Alignment>&) { }

	~aligned_allocator() { }


	// The following will be different for each allocator.
	T* allocate(const std::size_t n) const
	{
		// The return value of allocate(0) is unspecified.
		// Mallocator returns NULL in order to avoid depending
		// on malloc(0)'s implementation-defined behavior
		// (the implementation can define malloc(0) to return NULL,
		// in which case the bad_alloc check below would fire).
		// All allocators can return NULL in this case.
		if (n == 0) {
			return NULL;
		}

		// All allocators should contain an integer overflow check.
		// The Standardization Committee recommends that std::length_error
		// be thrown in the case of integer overflow.
		if (n > max_size())
		{
			throw std::length_error("aligned_allocator<T>::allocate() - Integer overflow.");
		}

		// Mallocator wraps malloc().
		void* const pv = _mm_malloc(n * sizeof(T), Alignment);

		// Allocators should throw std::bad_alloc in the case of memory allocation failure.
		if (pv == NULL)
		{
			throw std::bad_alloc();
		}

		return static_cast<T*>(pv);
	}

	void deallocate(T* const p, const std::size_t n) const
	{
		_mm_free(p);
	}


	// The following will be the same for all allocators that ignore hints.
	template <typename U>
	T* allocate(const std::size_t n, const U* /* const hint */) const
	{
		return allocate(n);
	}


	// Allocators are not required to be assignable, so
	// all allocators should have a private unimplemented
	// assignment operator. Note that this will trigger the
	// off-by-default (enabled under /Wall) warning C4626
	// "assignment operator could not be generated because a
	// base class assignment operator is inaccessible" within
	// the STL headers, but that warning is useless.
private:
	aligned_allocator& operator=(const aligned_allocator&);
};

struct Program {
	uint32_t free_reg = 0;
	vector<uint64_t> instructions;
	vector<double> stack; // program constant storage goes at the bottom of the stack for now
	unordered_map<string, uint8_t> variables; // name => stackpos
	vector<Program> functions; // TODO: questionable style, fix it

	string to_string() {
		auto s = stringstream {};

		s << "  Bytecode:\n";
		for (auto bc : instructions) {
			auto opcode = uint16_t(bc >> 48);
			auto r1 = uint16_t(bc >> 32);
			auto r2 = uint16_t(bc >> 16);
			auto r3 = uint16_t(bc);
			s << "    " << opcode_to_string[(Opcode)opcode] << ", " << r1 << ", " << r2 << ", " << r3 << "\n";
		}

		s << "  Storage: [ ";
		for (auto& x : stack) {
			s << x << ", ";
		}
		s << "]\n";

		return s.str();
	}
};

struct BytecodeInterpreter {
	BytecodeInterpreter() {}
	void compile(const AstNode& node, Program& program) {
		auto _encode = [](Opcode opcode, uint64_t i1 = 0, uint64_t i2 = 0, uint64_t i3 = 0) {
			if (i1 > UINT16_MAX || i2 > UINT16_MAX || i3 > UINT16_MAX) throw runtime_error("Out of range instruction");
			return (
				uint64_t(opcode) << 48 |
				uint64_t(i1) << 32 |
				uint64_t(i2) << 16 |
				uint64_t(i3)
			);
		};
		#define encode(op, ...) _encode(Opcode::op, __VA_ARGS__)
		#define emit(op, ...) program.instructions.push_back(encode(op, __VA_ARGS__));
		#define rewrite(instr, op, ...) program.instructions[instr] = encode(op, __VA_ARGS__);
		#define handle(type, body) break; case AstType::type:
		#define child(n) compile(node.children[n], program);
		#define label(name) auto name = program.instructions.size();
		#define label_reg(name) auto name = program.free_reg - 1;

		switch (node.type) {
			case AstType::Unknown: return;
			
			handle(StatList) {
				for (auto i = 0; i < node.children.size(); i++) {
					child(i);
				}
			}
			handle(VarDeclStat) {
				child(1); // pushes 1 value
				// auto& stackpos = program.variables[node.children[0].data_s];
				auto& ident = node.children[0].data_s;
				auto iter = program.variables.find(ident);
				if (iter != program.variables.end()) {
					throw runtime_error("Compile error: variable already exists:" + ident);
				}
				auto stackpos = program.stack.size();
				emit(STORE, program.free_reg - 1, stackpos);
				program.variables[ident] = stackpos;
				program.stack.push_back(0);
				program.free_reg--;
				// create a stack entry
			}
			handle(AssignStat) {
				child(1);
				auto& ident = node.children[0].data_s;
				auto iter = program.variables.find(ident);
				if (iter == program.variables.end()) {
					throw runtime_error("Compile error: variable not found: " + ident);
				}
				emit(STORE, program.free_reg - 1, iter->second);
				program.free_reg--;
			}
			handle(PrintStat) {
				child(0);
				emit(PRINT, program.free_reg - 1);
				program.free_reg--;
			}
			handle(IfStat) {
				child(0); // evaluate the expression
				
				label(cond);
				label_reg(cond_reg);
				emit(CONDJUMP, 0, 0, 0); // PLACEHOLDER

				label(startif);
				child(1);
				label(endif);
				emit(JUMP, 0, 0, 0); // PLACEHODLER

				label(startelse);
				if (node.children.size() == 3) {
					child(2);
				}
				label(endelse);
				
				rewrite(cond, CONDJUMP, cond_reg, startif, startelse);
				rewrite(endif, JUMP, endelse);
			}
			handle(WhileStat) {
				label(eval_cond);
				child(0); // evaluate expression
				
				label(cond);
				label_reg(cond_reg);
				emit(CONDJUMP, 0, 0, 0); // placeholder

				label(startwhile);
				child(1);
				emit(JUMP, eval_cond);
				label(endwhile);

				rewrite(cond, CONDJUMP, cond_reg, startwhile, endwhile);
			}
			handle(ReturnStat) {}
			
			handle(TernaryExp) {}
			handle(OrExp) {}
			handle(AndExp) {}
			handle(BitOrExp) {}
			handle(BitAndExp) {}
			handle(BitXorExp) {}
			handle(EqExp) {
				child(0); child(1);
				emit(EQUAL, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(NotEqExp) {
				child(0); child(1);
				emit(NEQUAL, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(LessExp) {
				child(0); child(1);
				emit(LESS, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(GreaterExp) {
				child(0); child(1);
				emit(GREATER, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(LessEqExp) {
				child(0); child(1);
				emit(LESSEQ, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(GreaterEqExp) {
				child(0); child(1);
				emit(GREATEREQ, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(ShiftLeftExp) {}
			handle(ShiftRightExp) {}
			handle(AddExp) {
				child(0); child(1);
				emit(ADD, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			};
			handle(SubExp) {
				child(0); child(1);
				emit(SUB, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(MulExp) {
				child(0); child(1);
				emit(MUL, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(DivExp) {
				child(0); child(1);
				emit(DIV, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(ModExp) {
				child(0); child(1);
				emit(MOD, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}				
			handle(PowExp) {
				child(0); child(1);
				emit(POW, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(NegateExp) {
				child(0);
				emit(NEGATE, program.free_reg - 1);
			}
			handle(NotExp) {
				child(0);
				emit(NOT, program.free_reg - 1);
			}
			handle(BitNotExp) {
				child(0);
				emit(BITNOT, program.free_reg - 1);
			}

			handle(CallExp) {}
			handle(ArgList) {}
			handle(IndexExp) {}
			handle(AccessExp) {}
			handle(FuncLiteral) {
				//auto subprogram = Program{};
				/*compile(node.children[2], subprogram);
				program.functions.push_back(subprogram);
				program.stack.push_back(program.functions.size());
				emit(LOAD, program.free_reg, )*/
			}
			handle(ParamDef) {}
			handle(NumLiteral) {
				// TODO: don't store the same constants twice
				program.stack.push_back(node.data_d);
				emit(LOAD, program.free_reg, program.stack.size() - 1);
				program.free_reg++;
			}
			handle(StringLiteral) {}
			handle(ArrayLiteral) {}
			handle(ObjectLiteral) {}
			handle(Identifier) {
				// should only be encountered when compiling sub-expressions
				auto iter = program.variables.find(node.data_s);
				if (iter == program.variables.end()) {
					throw runtime_error("Unknown variable: " + node.data_s);
				}
				emit(LOAD, program.free_reg, iter->second);
				program.free_reg++;
			}
		}

		#undef encode
		#undef emit
		#undef handle
		#undef child
		// TODO: constant folding
		// TODO: jump folding
	}
	void execute(Program& program) {
		// TODO: optimization, computed goto, direct threading, ssa, ...
		auto registers = array<double, numeric_limits<uint16_t>::max()>{};
		auto instruction_pointer = 0;
		while (instruction_pointer < program.instructions.size()) {
			// decode
			auto instruction = program.instructions[instruction_pointer++];
			auto opcode = uint16_t(instruction >> 48);
			auto r1 = uint16_t(instruction >> 32);
			auto r2	= uint16_t(instruction >> 16);
			auto r3 = uint16_t(instruction);

#define handle(c) break; case (uint16_t)Opcode::c:
			
			// execute
			switch (opcode) {
				case (uint16_t)Opcode::UNKNOWN:
				handle(ADD)			registers[r1] = registers[r1] + registers[r2];
				handle(SUB)			registers[r1] = registers[r1] - registers[r2];
				handle(MUL)			registers[r1] = registers[r1] * registers[r2];
				handle(DIV)			registers[r1] = registers[r1] / registers[r2];
				handle(MOD)			registers[r1] = (double)(((uint64_t)registers[r1]) % ((uint64_t)registers[r2]));
				
				handle(EQUAL)		registers[r1] = registers[r1] == registers[r2];
				handle(NEQUAL)		registers[r1] = registers[r1] != registers[r2];
				handle(LESS)		registers[r1] = registers[r1] < registers[r2];
				handle(GREATER)		registers[r1] = registers[r1] > registers[r2];
				handle(LESSEQ)		registers[r1] = registers[r1] <= registers[r2];
				handle(GREATEREQ)	registers[r1] = registers[r1] >= registers[r2];
				
				handle(NEGATE)		registers[r1] = -registers[r1];
				
				handle(LOAD)		registers[r1] = program.stack[r2];
				handle(STORE)		program.stack[r2] = registers[r1];
				handle(PRINT)		log<true, false>("(print_bc)", registers[r1]);
				handle(JUMP)		instruction_pointer = r1;
				handle(CONDJUMP)	instruction_pointer = registers[r1] ? r2 : r3;
				break; default: throw runtime_error("unknown instruction "s + opcode_to_string[(Opcode)opcode]);
			}
		}
	}
};


int main(int argc, char* argv[]) {
	auto check_arg = [&](const string& s) {
		for (auto i = 0; i < argc; i++) {
			if (argv[i] == s) {
				return true;
			}
		}
		return false;
	};
	std::ios::sync_with_stdio(false);
	auto fname = (argc >= 2) ? argv[1] : "../../../test/source.pl5";
	auto s = read_text_file(fname);
	if (!s.has_value()) {
		throw runtime_error("error opening source file: "s + argv[1]);
	}
	auto source = s.value();
	auto out_ast = AstNode {};
	try {
		parse(source, out_ast);
		try {
			if (check_arg("-A")) {
				log("AST: ", out_ast.tostring());
			}

			if (check_arg("--twi")) {
				log("Running with reference TWI");
				auto vm = RefInterpreter {};
				auto before = steady_clock::now();
				auto e = vm.eval(out_ast);
				auto after = steady_clock::now();
				log("Evaling", e);
				log("TWI done in ", duration_cast<microseconds>(after - before).count() * 1e-6, "s");
			}
			
			log("Compiling");
			auto vm2 = BytecodeInterpreter();
			auto program = Program {};
			vm2.compile(out_ast, program);
			if (check_arg("-D")) {
				log("Program:\n", program.to_string());
			}

			auto before = steady_clock::now();
			vm2.execute(program);
			auto after = steady_clock::now();
			log("BC done in ", duration_cast<microseconds>(after - before).count() * 1e-6, "s");
			log("hello world");
		} catch (exception& e) {
			log("runtime error: ", e.what());
		}
	} catch (exception& e) {
		log("parser error: ", e.what());
	}

	if (argc < 2) {
		getchar();
	}

	return 0;
}
