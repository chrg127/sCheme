programname := scheme
# can be: debug, release
build := debug

files := scheme.c ht.c

CC := gcc
CFLAGS := -Wall -Wextra -pedantic -I. -std=c11
flags_deps = -MMD -MP -MF $(@:.o=.d)

ifeq ($(build),debug)
    outdir := debug
    CFLAGS += -g -DDEBUG
else ifeq ($(build),release)
    outdir := release
    CFLAGS += -O3
else
	$(error error: invalid value for variable 'build')
endif

all: $(outdir)/$(programname)

objs := $(patsubst %,$(outdir)/%.o,$(files))

$(outdir)/$(programname): $(outdir) $(objs)
	$(info Linking $@ ...)
	@$(CXX) $(objs) -o $@ $(LDLIBS)

-include $(outdir)/*.d

$(outdir)/%.c.o: %.c
	$(info Compiling $< ...)
	@$(CC) $(CFLAGS) $(flags_deps) -c $< -o $@

$(outdir):
	mkdir -p $(outdir)

debug/test:
	mkdir -p debug
	mkdir -p debug/test

.PHONY: clean tests

clean:
	rm -rf debug release
