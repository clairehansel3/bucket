.PHONY: all build clean

BUCKETSOURCES = .build/abstract_syntax_tree.o \
								.build/code_generator.o \
								.build/lexer.o \
								.build/main.o \
								.build/miscellaneous.o \
								.build/parser.o \
								.build/run_compiler.o \
								.build/source_file.o \
								.build/symbol_table.o \
								.build/token.o

FLAGS = -DNDEBUG -DBUCKET_EXCEPTION_STACKTRACE -std=c++17 -g -O0 \
	-fsanitize=undefined,address -DBUCKET_DEBUG \
								-isystem $(shell llvm-config --includedir) \
								-isystem /usr/local/include \
								-Weverything -Wno-c++98-compat -Wno-padded \
								-Wno-shadow-field-in-constructor \
								-Wno-c++98-compat-pedantic \
								-Wno-exit-time-destructors \
								-Wno-weak-vtables \
								-Wno-return-std-move-in-c++11 \
								-Wno-float-equal \
								-Wno-reserved-id-macro
LINKFLAGS  := $(FLAGS) -L $(shell llvm-config --libdir)
LIBRARIES  := -lboost_program_options \
							$(shell llvm-config --libs) \
							$(shell llvm-config --system-libs)

BUILTINFLAGS = -std=c++17 -O3 -DBUCKET_BUILTIN_UBCKECK -Weverything -Wno-c++98-compat -Wno-padded \
-Wno-shadow-field-in-constructor \
-Wno-c++98-compat-pedantic \
-Wno-exit-time-destructors \
-Wno-weak-vtables \
-Wno-return-std-move-in-c++11 \
-Wno-float-equal \
-Wno-reserved-id-macro

all: build bucket libbuiltin.a

build:
	@ mkdir -p .build

libbuiltin.a: .build/builtin.o
	@ echo axv libbuiltin
	@ ar cr libbuiltin.a .build/builtin.o

bucket: $(BUCKETSOURCES)
	@ echo lnk bucket
	@ /usr/bin/clang++ -frtti $(LINKFLAGS) $(BUCKETSOURCES) $(LIBRARIES) -o bucket

.build/builtin.o: code/builtin.cxx
	@ echo cxx builtin.cxx
	@ /usr/bin/clang++ $(BUILTINFLAGS) -c code/builtin.cxx -o .build/builtin.o

.build/abstract_syntax_tree.o: code/abstract_syntax_tree.cxx
	@ echo cxx abstract_syntax_tree.cxx
	@ /usr/bin/clang++ -fno-rtti $(FLAGS) -c code/abstract_syntax_tree.cxx -o .build/abstract_syntax_tree.o

.build/code_generator.o: code/code_generator.cxx
	@ echo cxx code_generator.cxx
	@ /usr/bin/clang++ -fno-rtti $(FLAGS) -c code/code_generator.cxx -o .build/code_generator.o

.build/lexer.o: code/lexer.cxx
	@ echo cxx lexer.cxx
	@ /usr/bin/clang++ -fno-rtti $(FLAGS) -c code/lexer.cxx -o .build/lexer.o

.build/main.o: code/main.cxx
	@ echo cxx main.cxx
	@ /usr/bin/clang++ -frtti $(FLAGS) -c code/main.cxx -o .build/main.o

.build/miscellaneous.o: code/miscellaneous.cxx
	@ echo cxx miscellaneous.cxx
	@ /usr/bin/clang++ -fno-rtti $(FLAGS) -c code/miscellaneous.cxx -o .build/miscellaneous.o

.build/parser.o: code/parser.cxx
	@ echo cxx parser.cxx
	@ /usr/bin/clang++ -fno-rtti $(FLAGS) -c code/parser.cxx -o .build/parser.o

.build/run_compiler.o: code/run_compiler.cxx
	@ echo cxx run_compiler.cxx
	@ /usr/bin/clang++ -fno-rtti $(FLAGS) -c code/run_compiler.cxx -o .build/run_compiler.o

.build/source_file.o: code/source_file.cxx
	@ echo cxx source_file.cxx
	@ /usr/bin/clang++ -fno-rtti $(FLAGS) -c code/source_file.cxx -o .build/source_file.o

.build/symbol_table.o: code/symbol_table.cxx
	@ echo cxx symbol_table.cxx
	@ /usr/bin/clang++ -fno-rtti $(FLAGS) -c code/symbol_table.cxx -o .build/symbol_table.o

.build/token.o: code/token.cxx
	@ echo cxx token.cxx
	@ /usr/bin/clang++ -fno-rtti $(FLAGS) -c code/token.cxx -o .build/token.o

clean:
	@ echo cln
	@ rm -rf .build
	@ rm bucket libbuiltin.a
