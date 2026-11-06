# Benchmark Results Analysis

## 1. Experimental setup

The benchmarks compare sequential and OpenMP implementations of three image-processing operations:

- Grayscale conversion
- Fixed thresholding, using a threshold of 128
- Otsu's automatic thresholding

All operations were tested on the same RGB image:

| Property | Value |
|---|---:|
| Image dimensions | 3648 × 5472 |
| Number of pixels | 19,961,856 |
| Channels | 3 (RGB) |
| Repetitions per configuration | 10 |
| Parallel configurations | 1–8 threads |

The reported execution times are averages over 10 repetitions. Speedup is calculated as

$$
S(p) = \frac{T_{\text{sequential}}}{T_{\text{parallel}}(p)}
$$

where $p$ is the number of OpenMP threads. Efficiency is

$$E(p) = \frac{S(p)}{p} \times 100\%$$

The ideal speedup line is $S(p)=p$. It represents perfect linear scaling: doubling the number of threads would halve the execution time.

All three operations passed the output comparison: the sequential and parallel output images were identical. Otsu also selected the same threshold in both implementations (110).

## 2. Results overview

| Operation | Sequential time | 1-thread time | 8-thread time | 8-thread speedup |
|---|---:|---:|---:|---:|
| Grayscale | 28.20 ms | 28.80 ms | 7.00 ms | 4.03× |
| Fixed threshold | 34.00 ms | 34.20 ms | 6.60 ms | 5.15× |
| Otsu thresholding | 89.80 ms | 66.10 ms | 13.60 ms | 6.60× |

The results show that all three operations benefit from parallel execution as the thread count increases. The 8-thread configuration is the fastest configuration for each operation in this experiment.

### Grayscale conversion

Grayscale conversion achieves a speedup of approximately 4.03× with 8 threads. Its execution time decreases from 28.20 ms sequentially to 7.00 ms with 8 threads.

The speedup generally increases as more threads are used, although it does not scale linearly. For example, the 5-thread configuration reaches 3.17×, while the 6-thread configuration drops slightly to 2.94×. The result at 7 threads recovers to 3.13×.

These small fluctuations are normal in timing experiments. Thread scheduling, operating-system activity, memory bandwidth, CPU frequency changes, and other background activity can affect individual measurements. The 8-thread result is the best in this run, but the intermediate results are not perfectly monotonic.

### Fixed thresholding

Fixed thresholding achieves the strongest scaling of the two simpler operations, reaching 5.15× speedup with 8 threads. Execution time falls from 34.00 ms to 6.60 ms.

The speedup increases at every tested thread count in this run. However, scaling is still sublinear: 8 threads do not provide an 8× speedup. This is expected because parallel execution has overhead, and the operation is also constrained by memory access and memory bandwidth.

### Otsu thresholding

Otsu achieves the highest reported 8-thread speedup: approximately 6.60×. Its execution time decreases from 89.80 ms sequentially to 13.60 ms with 8 threads.

The Otsu result has an important distinction: its parallel implementation is already considerably faster than its sequential implementation when using only one thread. The implications of this are discussed in the next section. Therefore, its speedup figures should not yet be interpreted as a clean measurement of parallel scaling.

## 3. Why does the Otsu speedup exceed the ideal line?

The Otsu speedup values are:

| Threads | Speedup | Ideal speedup |
|---:|---:|---:|
| 1 | 1.36× | 1× |
| 2 | 2.65× | 2× |
| 3 | 3.85× | 3× |
| 4 | 4.85× | 4× |
| 5 | 4.85× | 5× |
| 6 | 5.51× | 6× |
| 7 | 6.24× | 7× |
| 8 | 6.60× | 8× |

A measured speedup above the ideal \(S(p)=p\) line is called **superlinear speedup**. It is possible in parallel computing. For example, splitting a problem across processors can sometimes improve cache behavior enough that the combined execution time decreases by more than the number of processors alone would suggest.

However, the Otsu measurements show a warning sign: the parallel implementation with just **one thread takes 66.10 ms**, while the sequential implementation takes **89.80 ms**.

A one-thread OpenMP run is not expected to be substantially faster simply because it uses OpenMP. In this case, the one-thread parallel version is about 26.4% faster than the sequential version:

$$
\frac{89.80}{66.10} \approx 1.36.
$$

That initial difference inflates every Otsu speedup value because all of them use the same 89.80 ms sequential baseline. For example, the 8-thread speedup is calculated as

$$
\frac{89.80}{13.60} \approx 6.60.
$$

If we instead compare the 8-thread execution time against the one-thread parallel implementation, the ratio is

$$
\frac{66.10}{13.60} \approx 4.86.
$$

This is not the standard sequential-to-parallel speedup, but it helps show how much of the reported 6.60× comes from the initial difference between the two implementations.

### Does this mean the code is wrong?

Not necessarily, but it means the Otsu benchmark is not yet a fair comparison of sequential and parallel performance.

Possible causes include:

1. **Different implementation paths:** The sequential and parallel Otsu functions may perform different amounts of work or use different operations, even if they produce identical output and select the same threshold.
2. **Compiler optimization differences:** The compiler may optimize the two code paths differently.
3. **Measurement effects:** CPU frequency changes, cache state, and background activity can affect timings. Ten repetitions reduce some noise, but do not eliminate systematic differences.
4. **Different histogram construction strategies:** The parallel version uses per-thread histograms and combines them, while the sequential version builds one histogram. These should be logically equivalent, but their implementation details may lead to different performance.

The first thing to investigate is the implementation itself. Check that both versions:
- Calculate grayscale intensity in the same way.
- Visit every pixel exactly once when building the histogram.
- Use the same Otsu threshold-selection calculation.
- Apply the selected threshold to every pixel in the same way.
- Include all required work in the timed region.

Also make sure that the sequential and parallel implementations are both compiled with the same optimization flags. The current benchmark does this when they are built in the same executable, but differences in the functions themselves can still matter.

After checking the implementations, run more repetitions and consider alternating the order in which sequential and parallel measurements are taken. This can help reduce bias caused by changing CPU conditions.

**Conclusion:** Superlinear speedup is possible, but the Otsu result should be treated as suspicious until the 1-thread parallel implementation and sequential implementation are checked for equivalent work. The output comparison confirms correctness of the resulting image; it does not prove that the two implementations perform exactly the same amount of work.

## 4. Why is speedup below the ideal line for the other operations?

Grayscale and fixed thresholding both show the expected sublinear scaling. This is normal.

The ideal line assumes that all work can be divided perfectly between threads and that adding threads introduces no overhead. Real systems do not behave that way.

Some of the main limitations are:

- **OpenMP overhead:** Creating and coordinating a parallel team takes time.
- **Memory bandwidth:** Each thread reads and writes image data. Eventually, memory access can become the bottleneck.
- **Uneven scheduling and system activity:** Threads may not receive exactly equal effective execution time.
- **Serial work:** Any work that cannot be parallelized limits the maximum speedup.

For these reasons, adding more threads usually produces diminishing returns rather than perfectly linear speedup.

## 5. Correctness

All three benchmark records report:

| Operation | Outputs identical | Thresholds identical |
|---|---|---|
| Grayscale | Yes | Yes |
| Fixed threshold | Yes | Yes |
| Otsu | Yes | Yes |

The output comparison is important because performance improvements are only useful if the parallel implementation preserves the expected result.

For Otsu, the selected threshold was 110 in both implementations, and the generated output images were identical.

## 6. Conclusion

For this image and this test environment, the 8-thread configuration produced the lowest execution time for all three operations. Fixed thresholding reached 5.15× speedup, grayscale reached 4.03×, and Otsu reported 6.60×.

The grayscale and fixed-threshold results show plausible sublinear scaling. The Otsu result appears superlinear for thread counts 1–4, but the fact that its one-thread parallel implementation is already 1.36× faster than its sequential implementation suggests that the baseline comparison needs further investigation.

The next step should be to inspect and align the sequential and parallel Otsu implementations before making a strong claim about its parallel speedup.
