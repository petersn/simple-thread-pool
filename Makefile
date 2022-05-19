
CXXFLAGS=-Wall -Wextra

user: user.o
	$(CXX) -o $@ $<

