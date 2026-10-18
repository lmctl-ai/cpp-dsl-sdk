// Conformance runner: implements the host stub described in conformance/vectors/manifest.json ("host") and checks every
// NAME.dslb against NAME.expect. A second runtime (Rust, R21) implements the same stub and reads the same files.
// Usage: build/conformance conformance/vectors
#include "dslvm.hpp"
#include <algorithm>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

struct Thing { int id; };
struct Stub {
  std::vector<std::string> trace; std::vector<std::unique_ptr<Thing>> things; Thing root{0};
  dslvm::Registry reg;
  Thing* make(int id) { things.push_back(std::make_unique<Thing>(Thing{id})); return things.back().get(); }
  static std::string fmt(double f) { char buf[64]; std::snprintf(buf, sizeof buf, "%.15g", f); return buf; }
  Stub() {
    using namespace dslvm;
    reg.context("t", "Trace"); reg.context("root", "Thing");
    reg.method("Trace", "int", 1, [&](void*, Args& a) { trace.push_back("int " + std::to_string(a[0].i)); return Value::none(); });
    reg.method("Trace", "float", 1, [&](void*, Args& a) { trace.push_back("float " + fmt(a[0].f)); return Value::none(); });
    reg.method("Trace", "str", 1, [&](void*, Args& a) { trace.push_back("str " + a[0].s); return Value::none(); });
    reg.method("Trace", "bool", 1, [&](void*, Args& a) { trace.push_back(std::string("bool ") + (a[0].b ? "true" : "false")); return Value::none(); });
    reg.method("Trace", "thing", 1, [&](void*, Args& a) { trace.push_back("thing " + std::to_string(static_cast<Thing*>(a[0].obj)->id)); return Value::none(); });
    reg.method("Thing", "id", 0, [&](void* self, Args&) { return Value::integer(static_cast<Thing*>(self)->id); });
    reg.method("Thing", "next", 0, [&](void* self, Args&) { return Value::object("Thing", make(static_cast<Thing*>(self)->id + 1)); });
    reg.method("Thing", "name", 0, [&](void* self, Args&) { return Value::string("thing-" + std::to_string(static_cast<Thing*>(self)->id)); });
    reg.method("Thing", "scaled", 1, [&](void* self, Args& a) { return Value::real(static_cast<Thing*>(self)->id * a[0].f); });
    reg.function("thing", 1, [&](Args& a) { return Value::object("Thing", make(a[0].i)); });
    reg.function("twice", 1, [&](Args& a) { return Value::integer(a[0].i * 2); });
    reg.function("half", 1, [&](Args& a) { return Value::real(a[0].f / 2.0); });
    reg.function("yes", 0, [&](Args&) { trace.push_back("yes"); return Value::boolean(true); });
    reg.function("fail", 1, [&](Args& a) -> Value { throw Error(a[0].s); });
  }
};

static std::string unescape(const std::string& s) { std::string out; for (size_t i = 0; i < s.size(); ++i) { if (s[i] == '\\' && i + 1 < s.size() && s[i + 1] == 'n') { out += '\n'; ++i; } else out += s[i]; } return out; }

int main(int argc, char** argv) {
  if (argc != 2) { std::cerr << "usage: conformance VECTOR_DIR\n"; return 2; }
  std::string dir = argv[1];
  std::vector<std::string> names;
  if (DIR* d = ::opendir(dir.c_str())) { while (dirent* e = ::readdir(d)) { std::string n = e->d_name; if (n.size() > 7 && n.compare(n.size() - 7, 7, ".expect") == 0) names.push_back(n.substr(0, n.size() - 7)); } ::closedir(d); }
  std::sort(names.begin(), names.end());
  if (names.empty()) { std::cerr << "no vectors in " << dir << "\n"; return 2; }
  int failed = 0;
  for (const std::string& name : names) {
    std::ifstream ex(dir + "/" + name + ".expect"); std::string kind; std::getline(ex, kind); std::vector<std::string> lines; for (std::string l; std::getline(ex, l);) lines.push_back(unescape(l));
    std::string schemaFile = dir + "/schema.json";
    { std::ifstream m(dir + "/manifest.json"); std::stringstream ss; ss << m.rdbuf(); std::string mf = ss.str(); size_t p = mf.find("\"name\": \"" + name + "\""); if (p != std::string::npos && mf.find("\"schema\": \"schema-extra.json\"", p) < mf.find("\"name\": \"", p + 1)) schemaFile = dir + "/schema-extra.json"; }
    std::string bytes = dslvm::read_file(dir + "/" + name + ".dslb"), schema = dslvm::read_file(schemaFile);
    Stub stub; std::string result, error; bool loaded = false, ran = false;
    try { dslvm::Program p = dslvm::Program::load(bytes, schema, stub.reg); loaded = true; dslvm::Context c; c.set("t", &stub.trace); c.set("root", &stub.root); p.run(c, stub.reg); ran = true; }
    catch (const dslvm::Error& e) { error = e.what(); }
    bool ok;
    if (kind == "trace") ok = ran && stub.trace == lines;
    else if (kind == "runtime_error") ok = loaded && !ran && error.find(lines.at(0)) != std::string::npos;
    else if (kind == "load_error") ok = !loaded && error.find(lines.at(0)) != std::string::npos;
    else { std::cerr << name << ": unknown expectation kind '" << kind << "'\n"; return 2; }
    if (ok) std::cout << "ok - " << name << " (" << kind << ")\n";
    else { failed++; std::cout << "FAIL - " << name << " (" << kind << ")\n"; if (!error.empty()) std::cout << "    error: " << error << "\n"; for (auto& t : stub.trace) std::cout << "    got:  " << t << "\n"; for (auto& l : lines) std::cout << "    want: " << l << "\n"; }
  }
  std::cout << (failed ? "FAIL " : "PASS ") << (names.size() - failed) << "/" << names.size() << " vectors\n";
  return failed ? 1 : 0;
}
