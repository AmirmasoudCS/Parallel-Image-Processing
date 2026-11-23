# Parallel-Image-Processing

A C project that applies image-processing filters sequentially and in parallel using **OpenMP**. It implements grayscale conversion, binary thresholding, and Otsu binarization. It can either process a single image and save the result, or benchmark the sequential and parallel implementations across different thread counts.

## Features

- Load and save images using `stb_image` and `stb_image_write`.
- Apply grayscale conversion, binary thresholding, and Otsu binarization, sequentially and in parallel.
- **Process mode (default):** run the parallel filter once on an input image and save the result.
- **Benchmark mode (`--benchmark`):** compare sequential and parallel implementations over repeated runs.
- Choose the number of OpenMP threads from the command line (`--threads N`).
- Measure execution time, speedup, and parallel efficiency.
- Verify that sequential and parallel outputs are identical.
- Save benchmark results in JSONL format.
- Generate performance charts using Python.

## Project Structure

```text
.
├── images/
│   ├── output/              # Processed images
│   └── image1.jpg           # Input image
├── include/                 # Header files and stb libraries
├── results/
│   ├── charts/              # Generated benchmark charts
│   └── benchmark_results.jsonl
├── scripts/
│   └── plot_results.py      # Benchmark visualization
├── src/                     # C source files
├── LICENSE
├── README.md
├── requirements.txt
└── tree.txt
```

> Generated using [Tree Printer](https://github.com/AmirmasoudCS/Tree-Printer.git).

## Compile

Run these commands from the project root:

```powershell
New-Item -ItemType Directory -Force build

gcc -std=c11 -Wall -Wextra -O2 -fopenmp -Iinclude `
    src/main.c src/image.c src/filters.c src/benchmark.c src/stb_impl.c `
    -o build/image_processor.exe
```

## Usage

```text
image_processor <input> <output> <filter> [threshold] [--benchmark] [--threads N]
```

| Argument | Description |
|----------|-------------|
| `<input>` | Path to the input image (any format supported by `stb_image`, e.g. JPG, PNG, BMP). |
| `<output>` | Path for the output image. The file is always written in PNG format. |
| `<filter>` | `grayscale`, `threshold`, `otsu`, or `all`. |
| `[threshold]` | Optional integer from 0 to 255. Only valid with the `threshold` filter (default: 128). |
| `--benchmark`, `-b` | Benchmark sequential vs. parallel instead of just processing the image. |
| `--threads N` | Number of OpenMP threads to use (1 to 4096). If omitted, OpenMP's default is used (or `OMP_NUM_THREADS`, if set). |

### Process mode (default)

Without `--benchmark`, the program runs the parallel filter once and saves the result. No timing is performed and the sequential version is not run.

**Grayscale conversion:**

```powershell
.\build\image_processor.exe images/image1.jpg images/output/grayscale.png grayscale
```

**Binary thresholding (default threshold: 128):**

```powershell
.\build\image_processor.exe images/image1.jpg images/output/threshold.png threshold
```

**Binary thresholding with a custom threshold:**

```powershell
.\build\image_processor.exe images/image1.jpg images/output/threshold_180.png threshold 180
```

The threshold value must be an integer between 0 and 255.

**Otsu binarization** (the computed threshold is printed to the console):

```powershell
.\build\image_processor.exe images/image1.jpg images/output/otsu.png otsu
```

**Choosing the number of threads:**

```powershell
.\build\image_processor.exe images/image1.jpg images/output/grayscale.png grayscale --threads 4
```

**All filters:**

```powershell
.\build\image_processor.exe images/image1.jpg images/output/result.png all
```

With `all`, one output file is written per filter, with the filter name added to the output filename:

```text
images/output/result_grayscale.png
images/output/result_threshold.png    (threshold fixed at 128)
images/output/result_otsu.png
```

### Benchmark mode

Add `--benchmark` to any of the commands above to benchmark instead of just processing:

```powershell
.\build\image_processor.exe images/image1.jpg images/output/grayscale.png grayscale --benchmark
.\build\image_processor.exe images/image1.jpg images/output/result.png all --benchmark
```

For each filter, the program benchmarks the sequential implementation and then the parallel implementation using **1 to 8 threads**. It checks that the outputs match, saves the parallel result image, and appends the measurements to:

```text
results/benchmark_results.jsonl
```

## Generate Charts

Charts are built from the data collected in benchmark mode. Run at least one benchmark first.

Install the required Python packages:

```powershell
pip install -r requirements.txt
```

Generate the benchmark charts:

```powershell
python scripts/plot_results.py
```

The charts are saved in `results/charts/`, with separate charts for each filter.

## Results Analysis

For a detailed discussion of benchmark results, performance scaling,
and the observed speedup behavior, see
[Results Analysis](RESULTS_ANALYSIS.md).

## License

See [LICENSE](./LICENSE) for license information.