
CXXFLAGS=-Wall -Wextra -std=c++20 -O3

user: user.o
	$(CXX) -o $@ $< -pthread

speed_tests: speed_tests.o
	$(CXX) -o $@ $< -pthread

.PHONY: clean
clean:
	rm -f *.o user speed_tests
