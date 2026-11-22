CXX      ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -I.
LDLIBS   ?= -ldl

RT_SRCS := \
	main.cpp \
	client/client.cpp \
	node/node.cpp \
	node/dag.cpp \
	node/registry.cpp \
	runtime/executor.cpp \
	runtime/supervisor.cpp \
	env_source.cpp \
	demo/vec/program.cpp

OBJS := $(RT_SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

all: rt demo/vec/vec.so

rt: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

demo/vec/vec.so: demo/vec/env.cpp env_abi.h
	$(CXX) $(CXXFLAGS) -shared -fPIC -o $@ $<

-include $(DEPS)

clean:
	rm -f rt $(OBJS) $(DEPS)

.PHONY: all clean
