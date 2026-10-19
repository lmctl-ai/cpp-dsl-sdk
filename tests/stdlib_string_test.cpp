#include "../stdlib/string.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace dslvm::stdlib;
int main(int argc, char** argv) {
  dslvm::Registry reg; register_string(reg);
  if (argc == 2 && std::string(argv[1]) == "--signatures") {
    for (const auto& s : reg.signatures()) std::cout << s << '\n';
    return 0;
  }
  int passed = 0;
  auto check = [&](bool ok) { if (!ok) { std::cerr << "failed string check " << passed + 1 << '\n'; std::abort(); } ++passed; };
  check(str_starts_with("alpha", "al") && str_starts_with("", "") && !str_starts_with("a", "ab"));
  check(str_ends_with("alpha", "ha") && str_ends_with("", "") && !str_ends_with("a", "ba"));
  check(str_contains("abc", "") && str_contains("abc", "b") && !str_contains("abc", "z"));
  check(str_len(std::string("a\0b", 3)) == 3);
  check(str_substr("abcdef", -4, 2) == "ab" && str_substr("abcdef", 3, 100) == "def");
  check(str_substr("abc", 9, 3).empty() && str_substr("abc", 1, -1).empty());
  check(str_lower("AbC-\xC3\x89") == "abc-\xC3\x89");
  check(str_upper("aBc-\xC3\xA9") == "ABC-\xC3\xA9");
  check(str_find("abcabc", "bc") == 1 && str_find("abc", "z") == -1 && str_find("abc", "") == 0);
  check(str_replace("a-b-a", "a", "xy") == "xy-b-xy" && str_replace("abc", "", "x") == "abc");
  check(str_replace(std::string("a\0a", 3), std::string("\0", 1), "X") == "aXa");
  check(str_trim(" \tvalue\r\n") == "value" && str_trim("\xC2\xA0x\xC2\xA0") == "\xC2\xA0x\xC2\xA0");
  check(str_repeat("xy", 3) == "xyxyxy" && str_repeat("x", 0).empty() && str_repeat("", INT64_MAX).empty());
  bool repeat_refused = false; try { (void)str_repeat("ab", INT64_MAX); } catch (const dslvm::Error&) { repeat_refused = true; }
  check(repeat_refused);
  check(str_is_int("-9223372036854775808") && str_to_int("-9223372036854775808") == INT64_MIN && str_to_int("+12") == 12);
  check(!str_is_int("9223372036854775808") && !str_is_int(" 3") && !str_is_int("") && str_to_int("nope") == 0);
  check(str_is_float("-1.25e2") && std::abs(str_to_float("-1.25e2") + 125.0) < 1e-12 && str_to_float("+2.5") == 2.5);
  check(!str_is_float("nan") && !str_is_float("inf") && !str_is_float("1.2x") && str_to_float("bad") == 0.0);
  check(str_match("a/b.txt", "a/*.txt") && str_match("a", "[!b]") && str_match("x", "[z-a]") == false);
  check(str_match("?*[]\\", "\\?\\*\\[\\]\\\\") && str_match("[", "["));
  check(str_match("[", "["));
  check(str_match(std::string("a\0b", 3), std::string("a?b", 3)));
  std::string adversarial(12000, 'a');
  check(!str_match(adversarial, "*aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab"));
  // Binder signatures and an actual callback dispatch are covered in the host.
  auto entry = reg.function_by_signature("fn str_upper(string) -> string");
  dslvm::Args args{dslvm::Value::string("mixed")};
  check(entry && entry->fn(args).s == "MIXED");
  std::cout << "stdlib string: " << passed << " checks passed\n";
}
