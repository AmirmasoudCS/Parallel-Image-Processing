# Parallel-Image-Processing

A C project that applies image-processing filters sequentially and in parallel using **OpenMP**. It currently implements grayscale conversion and binary thresholding, with benchmarks across different thread counts.

## Features

- Load and save images using `stb_image` and `stb_image_write`.
- Apply grayscale conversion sequentially and in parallel.
- Apply binary thresholding sequentially and in parallel.
- Benchmark thread counts from 1 to 8.
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

## Run

The program accepts an input image, output path, filter name, and an optional threshold value.

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

**Otsu binarization:**

```powershell
.\build\image_processor.exe images/image1.jpg images/output/threshold_180.png otsu
```

**All filters:**

```powershell
.\build\image_processor.exe images/image1.jpg images/output/result.png all
```

For each filter, the program benchmarks the sequential implementation and then the parallel implementation using **1 to 8 threads**. It checks the outputs and appends the measurements to:

```text
results/benchmark_results.jsonl
```

## Generate Charts

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