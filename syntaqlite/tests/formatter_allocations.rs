//! Allocation regression checks for a warmed-up production formatter.
#![cfg(all(feature = "fmt", feature = "sqlite"))]
#![expect(unsafe_code, reason = "test-only allocator delegates to System")]
use std::alloc::{GlobalAlloc, Layout, System};
use std::cell::Cell;
use syntaqlite::Formatter;

struct CountingAllocator;
thread_local! {
    static ALLOCATIONS: Cell<Option<usize>> = const { Cell::new(None) };
}
fn allocated() {
    ALLOCATIONS.with(|count| {
        if let Some(n) = count.get() {
            count.set(Some(n + 1));
        }
    });
}
// SAFETY: Every allocation and deallocation is forwarded unchanged to System.
unsafe impl GlobalAlloc for CountingAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        allocated();
        // SAFETY: The caller supplies a valid allocation layout.
        unsafe { System.alloc(layout) }
    }
    unsafe fn alloc_zeroed(&self, layout: Layout) -> *mut u8 {
        allocated();
        // SAFETY: The caller supplies a valid allocation layout.
        unsafe { System.alloc_zeroed(layout) }
    }
    unsafe fn realloc(&self, ptr: *mut u8, layout: Layout, size: usize) -> *mut u8 {
        allocated();
        // SAFETY: The caller supplies the allocation and its valid new size.
        unsafe { System.realloc(ptr, layout, size) }
    }
    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        // SAFETY: The allocation came from this System-backed allocator.
        unsafe { System.dealloc(ptr, layout) }
    }
}
#[global_allocator]
static ALLOCATOR: CountingAllocator = CountingAllocator;

#[test]
fn repeated_formatting_only_allocates_the_returned_string() {
    let large = "select a, b from t where a > 1; -- comment\n".repeat(128);
    for source in [
        "select 1",
        "select a /* comment */, b from t -- predicate\nwhere a > 1",
        "create table t(x decimal(10 , 2) generated always as (1) stored)",
        large.as_str(),
    ] {
        let mut formatter = Formatter::new();
        let expected = formatter.format(source).expect("valid SQL");
        for after_error in [false, true] {
            if after_error {
                assert!(formatter.format("select from").is_err());
            }
            ALLOCATIONS.with(|count| count.set(Some(0)));
            let result = formatter.format(source);
            let allocations = ALLOCATIONS.with(|count| count.replace(None).expect("counting"));
            assert_eq!(result.expect("valid SQL"), expected);
            eprintln!(
                "{} bytes, after_error={after_error}: {allocations} Rust allocations",
                source.len()
            );
            assert_eq!(
                allocations, 1,
                "scratch buffers must survive reuse and errors"
            );
        }
    }
}
