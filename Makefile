.PHONY: all build clean
CXX = clang++
FLAGS = -std=c++17 \
				-isystem /usr/local/include \
				-isystem /Users/chansel/Desktop/things/prefix/include \
				-Weverything \
				-Wno-c++98-compat \
				-Wno-c++98-compat-pedantic \
				-Wno-date-time \
				-Wno-exit-time-destructors \
				-Wno-poison-system-directories \
				-Wno-padded \
				-Wno-reserved-id-macro \
				-Wno-weak-vtables \
				-frtti \
				-O0 \
				-g \
				-isystem /Users/chansel/Desktop/things/software/llvm_debug_prefix/include \
				-DBUCKET_DEBUG
LINKFLAGS = -L/Users/chansel/Desktop/things/software/llvm_debug_prefix/lib \
						-lboost_program_options \
						$(shell /Users/chansel/Desktop/things/software/llvm_debug_prefix/bin/llvm-config --libs) \
						$(shell /Users/chansel/Desktop/things/software/llvm_debug_prefix/bin/llvm-config --system-libs)
SOURCES = $(shell find src -name "*.cxx" | xargs -I % basename % .cxx | xargs -I % echo .build/%.o)
all: build bucket
build:
	@ mkdir -p .build
bucket: .build/bucket.a
	@ echo "[\033[1;32mlnk\033[m]" bucket
	@ $(CXX) $(FLAGS) $(LINKFLAGS) .build/bucket.a -o bucket
.build/bucket.a: $(SOURCES)
	@ echo "[\033[1;32maxv\033[m]" bucket
	@ ar cr .build/bucket.a $(SOURCES)
.build/%.o: src/%.cxx
	@ echo "[\033[1;32mcxx\033[m]" $(shell basename $<)
	@ $(CXX) $(FLAGS) -c $< -o $@
clean:
	@ echo "[\033[1;32mcln\033[m]"
	@ -rm -rf bucket .build
