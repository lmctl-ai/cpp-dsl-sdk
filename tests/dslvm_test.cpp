// Unit tests of the SDK runtime: a tiny in-test assembler builds valid and invalid bytecode; every verifier rejection,
// the run-time checks and the resource limits are exercised. Build and run: make test
#include "dslvm.hpp"
#include <cassert>
#include <cstdio>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

using namespace dslvm;
static int passed = 0;

struct Asm {
  std::vector<std::string> consts; std::string code; uint16_t locals = 0;
  uint16_t k(const std::string& s) { for (size_t i = 0; i < consts.size(); ++i) if (consts[i] == s) return uint16_t(i); consts.push_back(s); return uint16_t(consts.size() - 1); }
  Asm& u8(unsigned v) { code.push_back(char(v)); return *this; }
  Asm& u16(unsigned v) { code.push_back(char(v >> 8)); code.push_back(char(v)); return *this; }
  Asm& i32(int32_t v) { uint32_t u = uint32_t(v); for (int i = 3; i >= 0; --i) code.push_back(char((u >> (i * 8)) & 0xff)); return *this; }
  Asm& f64(double d) { uint64_t bits; std::memcpy(&bits, &d, 8); for (int i = 7; i >= 0; --i) code.push_back(char((bits >> (i * 8)) & 0xff)); return *this; }
  Asm& pushInt(int32_t v) { return u8(PUSH_INT).i32(v); }
  Asm& pushFloat(double v) { return u8(PUSH_FLOAT).f64(v); }
  Asm& pushStr(const std::string& s) { return u8(PUSH_STR).u16(k(s)); }
  Asm& pushBool(bool b) { return u8(PUSH_BOOL).u8(b ? 1 : 0); }
  Asm& ctx(const std::string& n) { return u8(LOAD_CTX).u16(k(n)); }
  Asm& fn(const std::string& n, unsigned argc) { return u8(CALL_FN).u16(k(n)).u8(argc); }
  Asm& method(const std::string& t, const std::string& n, unsigned argc) { return u8(CALL_METHOD).u16(k(t)).u16(k(n)).u8(argc); }
  Asm& halt() { return u8(HALT); }
  std::string file(const std::string& schema, uint16_t version = VERSION, const std::string& magic = "DSLB") const {
    std::string out = magic; out.push_back(char(version >> 8)); out.push_back(char(version)); out += sha256(schema);
    out.push_back(char(consts.size() >> 8)); out.push_back(char(consts.size()));
    for (auto& c : consts) { out.push_back(char(c.size() >> 8)); out.push_back(char(c.size())); out += c; }
    out.push_back(char(locals >> 8)); out.push_back(char(locals));
    uint32_t n = uint32_t(code.size()); for (int i = 3; i >= 0; --i) out.push_back(char((n >> (i * 8)) & 0xff));
    return out + code;
  }
};

static const std::string SCHEMA = "{\"dsl.schema\":1,\"name\":\"t\"}";  // only its bytes matter to the runtime
struct Thing { int id; };
struct Host {
  std::vector<std::string> trace; Thing root{0};
  Registry reg;
  Host() {
    reg.context("t", "Trace").context("root", "Thing");
    reg.method("Trace", "int", {"int"}, "void", [&](void*, Args& a) { trace.push_back("int " + std::to_string(a[0].i)); return Value::none(); });
    reg.method("Trace", "str", {"string"}, "void", [&](void*, Args& a) { trace.push_back("str " + a[0].s); return Value::none(); });
    reg.method("Thing", "id", {}, "int", [&](void* self, Args&) { return Value::integer(static_cast<Thing*>(self)->id); });
    reg.function("twice", {"int"}, "int", [&](Args& a) { return Value::integer(a[0].i * 2); });
    reg.function("boom", {}, "void", [&](Args&) -> Value { throw Error("boom from the host"); });
  }
  std::vector<std::string> run(const std::string& bytes) { Program p = Program::load(bytes, SCHEMA, reg); Context c; c.set("t", &trace); c.set("root", &root); trace.clear(); p.run(c, reg); return trace; }
};

static void expect_error(const std::string& label, const std::function<void()>& f, const std::string& needle) {
  try { f(); } catch (const Error& e) { if (std::string(e.what()).find(needle) == std::string::npos) { std::cerr << "FAIL " << label << ": got '" << e.what() << "', want '" << needle << "'\n"; std::exit(1); } passed++; std::cout << "ok - " << label << ": " << e.what() << "\n"; return; }
  std::cerr << "FAIL " << label << ": no error (wanted '" << needle << "')\n"; std::exit(1);
}
static void expect_trace(const std::string& label, Host& h, const std::string& bytes, const std::vector<std::string>& want) {
  auto got = h.run(bytes); if (got != want) { std::cerr << "FAIL " << label << ":\n"; for (auto& g : got) std::cerr << "  got  " << g << "\n"; for (auto& w : want) std::cerr << "  want " << w << "\n"; std::exit(1); }
  passed++; std::cout << "ok - " << label << "\n";
}

int main() {
  Host h;
  // Valid programs.
  { Asm a; a.ctx("t").pushInt(1).pushInt(2).u8(ADD_I).method("Trace", "int", 1).ctx("t").pushStr("a").pushStr("b").u8(CONCAT).method("Trace", "str", 1).halt(); expect_trace("arithmetic, concat, method calls", h, a.file(SCHEMA), {"int 3", "str ab"}); }
  { Asm a; a.locals = 1; a.pushInt(21).fn("twice", 1).u8(STORE_LOCAL).u16(0).ctx("t").u8(LOAD_LOCAL).u16(0).method("Trace", "int", 1).halt(); expect_trace("host function and locals", h, a.file(SCHEMA), {"int 42"}); }
  { Asm a; a.pushBool(true).u8(JUMP_IF_FALSE).i32(19).ctx("t").pushInt(1).method("Trace", "int", 1).u8(JUMP).i32(14).ctx("t").pushInt(2).method("Trace", "int", 1).halt(); expect_trace("forward jumps select a branch", h, a.file(SCHEMA), {"int 1"}); }
  { Asm a; a.ctx("t").ctx("root").method("Thing", "id", 0).method("Trace", "int", 1).ctx("t").pushFloat(2.5).u8(F2S).method("Trace", "str", 1).ctx("t").pushInt(7).pushInt(2).u8(DIV_I).u8(I2F).pushFloat(0.5).u8(ADD_F).u8(F2S).method("Trace", "str", 1).ctx("t").pushInt(3).pushInt(3).u8(EQ).u8(B2S).method("Trace", "str", 1).halt(); expect_trace("context object, floats, conversions, equality", h, a.file(SCHEMA), {"int 0", "str 2.5", "str 3.5", "str true"}); }
  { Asm a; a.halt(); Program p = Program::load(a.file(SCHEMA), SCHEMA, h.reg); assert(p.constant_count() == 0 && p.code_size() == 1); passed++; std::cout << "ok - empty program loads\n"; }
  // Loader rejections (before anything runs).
  { Asm a; a.halt(); expect_error("bad magic", [&] { Program::load(a.file(SCHEMA, VERSION, "XXXX"), SCHEMA, h.reg); }, "bad magic"); }
  { Asm a; a.halt(); expect_error("other version", [&] { Program::load(a.file(SCHEMA, 2), SCHEMA, h.reg); }, "unsupported bytecode version 2 (this runtime reads version 3)"); }
  { Asm a; a.halt(); expect_error("schema mismatch", [&] { Program::load(a.file(SCHEMA), SCHEMA + " ", h.reg); }, "schema mismatch"); }
  { Asm a; a.ctx("t").halt(); std::string f = a.file(SCHEMA); expect_error("truncated", [&] { Program::load(f.substr(0, f.size() - 3), SCHEMA, h.reg); }, "bytecode is truncated"); }
  { Asm a; a.halt(); expect_error("trailing bytes", [&] { Program::load(a.file(SCHEMA) + "x", SCHEMA, h.reg); }, "trailing byte"); }
  { Asm a; a.u8(PUSH_STR).u16(9).halt(); expect_error("constant index out of range", [&] { Program::load(a.file(SCHEMA), SCHEMA, h.reg); }, "constant index 9 out of range"); }
  { Asm a; a.u8(LOAD_LOCAL).u16(0).halt(); expect_error("local slot out of range", [&] { Program::load(a.file(SCHEMA), SCHEMA, h.reg); }, "local slot 0 out of range"); }
  { Asm a; a.ctx("nope").halt(); expect_error("undeclared context", [&] { Program::load(a.file(SCHEMA), SCHEMA, h.reg); }, "context variable 'nope' is not declared"); }
  { Asm a; a.fn("missing", 0).halt(); expect_error("unbound function", [&] { Program::load(a.file(SCHEMA), SCHEMA, h.reg); }, "host function 'missing' is not bound"); }
  { Asm a; a.pushInt(1).fn("twice", 2).halt(); expect_error("function arity", [&] { Program::load(a.file(SCHEMA), SCHEMA, h.reg); }, "takes 1 argument(s), bytecode passes 2"); }
  { Asm a; a.ctx("root").method("Thing", "name", 0).halt(); expect_error("unbound method", [&] { Program::load(a.file(SCHEMA), SCHEMA, h.reg); }, "method 'Thing.name' is not bound"); }
  { Asm a; a.ctx("root").pushInt(1).method("Thing", "id", 1).halt(); expect_error("method arity", [&] { Program::load(a.file(SCHEMA), SCHEMA, h.reg); }, "method 'Thing.id' takes 0 argument(s), bytecode passes 1"); }
  { Asm a; a.u8(JUMP).i32(1).halt(); expect_error("jump into an operand", [&] { Program::load(a.file(SCHEMA), SCHEMA, h.reg); }, "not an instruction start"); }
  { Asm a; a.u8(JUMP).i32(-5).halt(); expect_error("backward jump", [&] { Program::load(a.file(SCHEMA), SCHEMA, h.reg); }, "goes backwards"); }
  { Asm a; a.pushInt(1); expect_error("missing HALT", [&] { Program::load(a.file(SCHEMA), SCHEMA, h.reg); }, "does not end with HALT"); }
  { Asm a; a.u8(0x7f).halt(); expect_error("unknown opcode", [&] { Program::load(a.file(SCHEMA), SCHEMA, h.reg); }, "unknown opcode 127"); }
  { Asm a; a.u8(PUSH_BOOL).u8(2).halt(); expect_error("bad bool operand", [&] { Program::load(a.file(SCHEMA), SCHEMA, h.reg); }, "PUSH_BOOL operand must be 0 or 1"); }
  { Asm a; a.locals = 5000; a.halt(); expect_error("too many locals", [&] { Program::load(a.file(SCHEMA), SCHEMA, h.reg); }, "too many locals"); }
  // Run-time checks (a correct compiler never produces these; the VM stays defensive).
  { Asm a; a.pushInt(1).pushInt(0).u8(DIV_I).halt(); expect_error("int division by zero", [&] { h.run(a.file(SCHEMA)); }, "int division by zero"); }
  { Asm a; a.pushInt(2147483647).pushInt(1).u8(ADD_I).halt(); expect_error("int overflow", [&] { h.run(a.file(SCHEMA)); }, "int overflow"); }
  { Asm a; a.pushInt(INT32_MIN).u8(NEG_I).halt(); expect_error("negating the smallest int", [&] { h.run(a.file(SCHEMA)); }, "negating the smallest int"); }
  { Asm a; a.pushInt(1).pushStr("x").u8(EQ).halt(); expect_error("mixed-kind equality", [&] { h.run(a.file(SCHEMA)); }, "cannot compare int with string"); }
  { Asm a; a.pushInt(1).pushFloat(1.0).u8(ADD_F).halt(); expect_error("float op on int", [&] { h.run(a.file(SCHEMA)); }, "float operator applied to int and float"); }
  { Asm a; a.locals = 1; a.u8(LOAD_LOCAL).u16(0).halt(); expect_error("local read before store", [&] { h.run(a.file(SCHEMA)); }, "read before assignment"); }
  { Asm a; a.pushInt(1).u8(JUMP_IF_FALSE).i32(0).halt(); expect_error("non-bool condition", [&] { h.run(a.file(SCHEMA)); }, "condition is a int, not a bool"); }
  { Asm a; a.u8(POP).halt(); expect_error("stack underflow", [&] { h.run(a.file(SCHEMA)); }, "stack underflow"); }
  { Asm a; a.fn("boom", 0).halt(); expect_error("host error propagates", [&] { h.run(a.file(SCHEMA)); }, "boom from the host"); }
  { Asm a; a.ctx("t").halt(); Program p = Program::load(a.file(SCHEMA), SCHEMA, h.reg); Context empty; expect_error("context object missing at run", [&] { p.run(empty, h.reg); }, "context variable 't' was not provided by the host for this run"); }
  { Asm a; a.ctx("t").method("Thing", "id", 0).halt(); expect_error("method on the wrong object type", [&] { h.run(a.file(SCHEMA)); }, "method Thing.id called on a Trace"); }
  // S0' (spec v4 A02/A03): signatures, duplicate registrations, error codes, the result line.
  { auto sigs = h.reg.signatures(); std::vector<std::string> want = {"context root: Thing", "context t: Trace", "fn boom() -> void", "fn twice(int) -> int", "method Thing.id() -> int", "method Trace.int(int) -> void", "method Trace.str(string) -> void"}; assert(sigs == want); passed++; std::cout << "ok - signatures() lists the sorted canonical strings\n"; }
  { Registry dup; dup.function("f", {}, "void", [](Args&) { return Value::none(); }); expect_error("duplicate registration", [&] { dup.function("f", {}, "void", [](Args&) { return Value::none(); }); }, "registered twice"); }
  { Asm a; a.fn("missing", 0).halt(); try { Program::load(a.file(SCHEMA), SCHEMA, h.reg); } catch (const Error& e) { assert(e.code == "LOAD_UNBOUND"); passed++; std::cout << "ok - unbound names carry LOAD_UNBOUND\n"; } }
  { Asm a; a.halt(); try { Program::load(a.file(SCHEMA), SCHEMA + " ", h.reg); } catch (const Error& e) { assert(e.code == "LOAD_SCHEMA_MISMATCH"); passed++; std::cout << "ok - schema mismatch carries LOAD_SCHEMA_MISMATCH\n"; } }
  { Asm a; a.halt(); try { Program::load(a.file(SCHEMA, VERSION, "XXXX"), SCHEMA, h.reg); } catch (const Error& e) { assert(e.code == "LOAD_BAD_BYTECODE"); passed++; std::cout << "ok - malformed files carry LOAD_BAD_BYTECODE\n"; } }
  { Asm a; a.fn("boom", 0).halt(); try { h.run(a.file(SCHEMA)); } catch (const Error& e) { assert(e.code == "RUN_HOST"); passed++; std::cout << "ok - host-thrown errors carry RUN_HOST\n"; } }
  { Asm a; a.pushInt(1).pushInt(0).u8(DIV_I).halt(); try { h.run(a.file(SCHEMA)); } catch (const Error& e) { assert(e.code == "RUN_ERROR"); passed++; std::cout << "ok - VM errors carry RUN_ERROR\n"; } }
  { assert(report_error("load", Error("LOAD_UNBOUND", "x")) == 3); assert(report_error("run", Error("host said no")) == 3); assert(report_app_failure("nothing to do") == 4); assert(report_ok("42") == 0); assert(json_escape("a\"b\n") == "\"a\\\"b\\n\""); passed++; std::cout << "ok - result lines and exit codes 3 (load/run) and 4 (application)\n"; }
  // Resource limits.
  { Asm a; for (size_t i = 0; i <= MAX_STACK; ++i) a.pushInt(1); a.halt(); expect_error("operand stack limit", [&] { h.run(a.file(SCHEMA)); }, "operand stack exceeds 1024 values"); }
  { Asm a; a.pushStr(std::string(65535, 'x')); for (int i = 0; i < 9; ++i) a.u8(DUP).u8(CONCAT); a.halt(); expect_error("concatenation size limit", [&] { h.run(a.file(SCHEMA)); }, "string + result exceeds"); }
  std::cout << "PASS " << passed << " checks\n";
  return 0;
}
