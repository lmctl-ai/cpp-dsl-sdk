CXX ?= g++
CXXFLAGS ?= -std=c++17 -O1 -Wall -Wextra -pedantic
BUILD ?= build

.PHONY: test test-string-stdlib conformance all clean
all: test conformance

$(BUILD)/dslvm_test: tests/dslvm_test.cpp include/dslvm.hpp
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -I include -o $@ tests/dslvm_test.cpp

$(BUILD)/conformance: conformance/run.cpp include/dslvm.hpp
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -I include -o $@ conformance/run.cpp

test: $(BUILD)/dslvm_test
	$(BUILD)/dslvm_test
	$(MAKE) test-string-stdlib

$(BUILD)/stdlib_string_test: tests/stdlib_string_test.cpp stdlib/string.hpp include/dslvm.hpp
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -I include -o $@ tests/stdlib_string_test.cpp

$(BUILD)/stdlib_string_header.o: tests/stdlib_string_header.cpp stdlib/string.hpp include/dslvm.hpp
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -std=c++17 -Wall -Wextra -Werror -I include -c -o $@ tests/stdlib_string_header.cpp

test-string-stdlib: $(BUILD)/stdlib_string_test $(BUILD)/stdlib_string_header.o
	$(BUILD)/stdlib_string_test
	bash tests/string-signature-parity.sh "$(abspath $(BUILD)/stdlib_string_test)"

conformance: $(BUILD)/conformance
	$(BUILD)/conformance conformance/vectors

clean:
	rm -rf $(BUILD)
