# QuatEngine Benchmarks

Native benchmark probes live here. They are deterministic CTest executables that
report elapsed time and validate finite checksums instead of enforcing brittle
machine-specific timing thresholds.

Run locally:

```bash
cmake -S . -B build-bench -DQE_BUILD_DEMO=OFF -DQE_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-bench --target qe_math_benchmark
ctest --test-dir build-bench --output-on-failure -L benchmark
```

Current probes:

- `qe_math_benchmark`: quaternion SLERP/rotate and Vec3 normalization/cross-product workloads.
