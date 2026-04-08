ROOT := $(shell cd .. && pwd)
RUNTIME := $(ROOT)/syntaqlite-syntax
DIALECT := $(ROOT)/syntaqlite-syntax

CC := cc
CXX := c++
CFLAGS := -Wall -Wextra -Wpedantic -std=c11 -I.
CXXFLAGS := -Wall -Wextra -Wpedantic -std=c++17 -I.

# Relaxed warnings for generated amalgamation source.
AMALG_CFLAGS := \
	-Wno-int-conversion \
	-Wno-void-pointer-to-int-cast \
	-Wno-unused-variable \
	-Wno-unused-parameter \
	-Wno-comment

BUILDTOOLS := $(ROOT)/target/release/syntaqlite-buildtools

.PHONY: all clean amalgamation

all: select_columns select_columns_c

# Generate amalgamation files from buildtools.
syntaqlite_sqlite.h syntaqlite_sqlite.c: $(BUILDTOOLS)
	$(BUILDTOOLS) amalgamate \
		--dialect sqlite \
		--runtime-dir $(RUNTIME) \
		--dialect-dir $(DIALECT) \
		--output-dir .

$(BUILDTOOLS):
	cargo build --release --manifest-path $(ROOT)/syntaqlite-buildtools/Cargo.toml

amalgamation: syntaqlite_sqlite.h syntaqlite_sqlite.c

syntaqlite_sqlite.o: syntaqlite_sqlite.c syntaqlite_sqlite.h
	$(CC) $(CFLAGS) $(AMALG_CFLAGS) -c -o $@ $<

libsyntaqlite.a: syntaqlite_sqlite.o
	ar rcs $@ $^

select_columns: select_columns.cc libsyntaqlite.a
	$(CXX) $(CXXFLAGS) -o $@ $^

select_columns_c: select_columns.c libsyntaqlite.a
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f select_columns select_columns_c libsyntaqlite.a *.o
	rm -f syntaqlite_sqlite.h syntaqlite_sqlite.c
