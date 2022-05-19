
CXXFLAGS=-Wall -Wextra -std=c++17

user: user.o
	$(CXX) -o $@ $< -pthread

