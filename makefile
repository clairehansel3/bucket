#------------------------------------------------------------------------------#

# Project Details
EXECUTABLE := bucket
SOURCEDIR  := code
BUILDDIR   := .build
EXTENSION  := cxx

# LLVM
LLVMCONFIG := /Users/claire/Desktop/things/software/llvm-prefix/bin/llvm-config

# Compiler Options
COMPILER   := /usr/bin/clang++
FLAGS      := -std=c++17 -g -O0 -fsanitize=undefined,address \
							-I $(shell $(LLVMCONFIG) --includedir) \
							-L $(shell $(LLVMCONFIG) --libdir)
LIBRARIES  := -lboost_program_options \
							$(shell $(LLVMCONFIG) --libs) \
							$(shell $(LLVMCONFIG) --system-libs)

# Shell Utilities
RM         := /bin/rm
ECHO       := /bin/echo
MKDIR      := /bin/mkdir
FIND       := /usr/bin/find

#------------------------------------------------------------------------------#

.PHONY: all build clean

SOURCES := $(shell $(FIND) $(SOURCEDIR) -name "*.$(EXTENSION)" -exec $(ECHO) $(BUILDDIR)/{}.o \;)

all: build $(BUILDDIR)/$(EXECUTABLE)

build:
	@ $(MKDIR) -p $(BUILDDIR)
	@ $(FIND) $(SOURCEDIR) -type d -exec $(MKDIR) -p $(BUILDDIR)/{} \;

$(BUILDDIR)/$(EXECUTABLE): $(SOURCES)
	$(COMPILER) $(FLAGS) $(SOURCES) $(LIBRARIES) -o $(EXECUTABLE)

$(BUILDDIR)/%.$(EXTENSION).o: %.$(EXTENSION)
	$(COMPILER) $(FLAGS) -c $< -o $@

clean:
	@ $(RM) -r $(BUILDDIR)
	@ $(RM) $(EXECUTABLE)

#------------------------------------------------------------------------------#
