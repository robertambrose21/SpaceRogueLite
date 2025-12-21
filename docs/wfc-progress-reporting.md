# WFC Progress Reporting Investigation

## Overview

Investigation into adding progress reporting for large map generation using the fast-wfc library in `core/src/generation/wfcstrategy.cpp`.

## Current Architecture

### WFC Base Class (`wfc.hpp`)

The `WFC` class already has **public step-by-step methods**:

- `observe()` - Selects and collapses a cell with lowest entropy
- `propagate()` - Propagates constraints after observation
- Returns `ObserveStatus`: `success`, `failure`, or `to_continue`

The `run()` method is a simple loop:

```cpp
while (true) {
    ObserveStatus result = observe();
    if (result == failure) return std::nullopt;
    if (result == success) return wave_to_output();
    propagator.propagate(wave);
}
```

### TilingWFC Class (`tiling_wfc.hpp`)

- Wraps `WFC` but keeps the `wfc` member **private**
- Only exposes `run()` - no access to step-by-step methods
- This is the class used in `WFCStrategy`

## Options for Progress Reporting

### Option 1: Expose Step-by-Step Methods in TilingWFC

**Complexity:** Low
**Invasiveness:** Minimal changes to library

Add public wrappers in `tiling_wfc.hpp`:

```cpp
public:
    WFC::ObserveStatus observe() noexcept { return wfc.observe(); }
    void propagate() noexcept { wfc.propagate(); }
```

Then implement a custom run loop in `WFCStrategy`:

```cpp
std::optional<Array2D<WFCTileSet::WFCTile>> WFCStrategy::runAttempt(int seed) {
    auto wfcTiles = tileSet.getTiles();
    auto neighbours = tileSet.getNeighbours();

    TilingWFC<WFCTileSet::WFCTile> wfc(wfcTiles, neighbours, getHeight(), getWidth(), {false}, seed);

    generateMapEdge(wfc);
    generateRoomsAndPaths(wfc);

    int totalCells = getWidth() * getHeight();
    int steps = 0;

    while (true) {
        auto status = wfc.observe();
        if (status == WFC::success) {
            return wfc.get_result(); // Would need to add this method too
        }
        if (status == WFC::failure) {
            return std::nullopt;
        }
        wfc.propagate();

        if (++steps % 100 == 0) {
            float progress = (float)steps / totalCells * 100.0f;
            spdlog::info("Generation progress: {:.1f}%", progress);
        }
    }
}
```

**Pros:**
- Minimal library changes
- Full control over progress reporting granularity
- Can add visualization of partial results

**Cons:**
- Need to also expose `id_to_tiling()` or add a `get_result()` method
- Slightly more code in WFCStrategy

---

### Option 2: Add Callback to WFC::run()

**Complexity:** Medium
**Invasiveness:** Modifies core WFC class

Modify `WFC::run()` signature in `wfc.hpp`:

```cpp
std::optional<Array2D<unsigned>> run(
    std::function<void(unsigned current, unsigned total)> progress_cb = nullptr
) noexcept;
```

Implementation in `wfc.cpp`:

```cpp
std::optional<Array2D<unsigned>> WFC::run(
    std::function<void(unsigned, unsigned)> progress_cb
) noexcept {
    unsigned total = wave.height * wave.width;
    unsigned current = 0;

    while (true) {
        ObserveStatus result = observe();

        if (result == failure) return std::nullopt;
        if (result == success) return wave_to_output();

        propagator.propagate(wave);

        if (progress_cb) {
            progress_cb(++current, total);
        }
    }
}
```

Then propagate to `TilingWFC::run()`:

```cpp
std::optional<Array2D<T>> run(
    std::function<void(unsigned, unsigned)> progress_cb = nullptr
) {
    auto a = wfc.run(progress_cb);
    if (a == std::nullopt) return std::nullopt;
    return id_to_tiling(*a);
}
```

Usage in WFCStrategy:

```cpp
return wfc.run([](unsigned current, unsigned total) {
    if (current % 100 == 0) {
        spdlog::info("Generation progress: {:.1f}%", (float)current / total * 100.0f);
    }
});
```

**Pros:**
- Clean API
- Callback can be used for various purposes (logging, UI updates, cancellation)
- No changes needed in consuming code structure

**Cons:**
- More invasive library changes
- Adds `<functional>` dependency to header-only parts

---

### Option 3: Add Progress Member Variable

**Complexity:** Low
**Invasiveness:** Minimal

Add a public progress member to `WFC` class:

```cpp
public:
    unsigned steps_completed = 0;
    unsigned total_cells() const { return wave.height * wave.width; }
```

Update in `run()`:

```cpp
while (true) {
    // ... existing code ...
    ++steps_completed;
}
```

Then in `TilingWFC`, expose it:

```cpp
unsigned get_progress() const { return wfc.steps_completed; }
unsigned get_total() const { return height * width; }
```

**Pros:**
- Very simple implementation
- Can poll progress from another thread

**Cons:**
- Requires polling rather than push-based notification
- Less flexible than callback approach

---

## Recommendation

**Option 1** is recommended for the following reasons:

1. Minimal changes to the third-party library
2. Aligns with the existing public API design (observe/propagate are already public in WFC)
3. Gives full control over progress reporting in your code
4. Enables future features like:
   - Visualizing partial generation results
   - Cancellation support
   - Adaptive retry strategies

## Files to Modify

For Option 1:

| File | Change |
|------|--------|
| `third-party/fast-wfc` (conanfile.py) | May need to patch the source |
| `~/.conan2/.../tiling_wfc.hpp` | Add `observe()`, `propagate()`, `get_result()` wrappers |
| `core/src/generation/wfcstrategy.cpp` | Replace `wfc.run()` with custom loop |

## Progress Calculation Notes

- Each `observe()` call collapses exactly one cell
- Total iterations approximately equals `width * height` minus pre-set tiles (edges, rooms)
- For a 100x100 map, expect ~10,000 iterations
- Progress percentage: `steps / (width * height) * 100`
