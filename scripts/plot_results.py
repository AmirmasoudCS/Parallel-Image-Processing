import json
import argparse
from pathlib import Path
from collections import defaultdict
from statistics import mean, stdev

import matplotlib.pyplot as plt


def load_results(filename):
    with open(filename, "r", encoding="utf-8") as file:
        content = file.read().strip()

    if not content:
        raise ValueError("The results file is empty!")

    try:
        data = json.loads(content)

        if isinstance(data, dict):
            return [data]

        return data

    except json.JSONDecodeError:
        results = []

        for line in content.splitlines():
            if line.strip():
                results.append(json.loads(line))

        return results


def group_results(results):
    groups = defaultdict(list)

    for result in results:
        image = result["image"]

        key = (
            image["width"],
            image["height"],
            image["channels"]
        )

        groups[key].append(result)

    return groups


def calculate_statistics(records):
    sequential_times = [
        record["sequential_time"]
        for record in records
    ]

    sequential_average = mean(sequential_times)

    sequential_std = (
        stdev(sequential_times)
        if len(sequential_times) > 1 else 0
    )

    thread_data = defaultdict(list)

    for record in records:
        for result in record["parallel_results"]:
            thread_data[result["threads"]].append(result["time"])

    threads = sorted(thread_data.keys())

    parallel_times = []
    parallel_std = []
    speedups = []
    efficiencies = []

    for thread_count in threads:
        times = thread_data[thread_count]

        average_time = mean(times)

        std_time = (
            stdev(times)
            if len(times) > 1 else 0
        )

        speedup = sequential_average / average_time
        efficiency = speedup / thread_count * 100

        parallel_times.append(average_time)
        parallel_std.append(std_time)
        speedups.append(speedup)
        efficiencies.append(efficiency)

    return {
        "sequential_time": sequential_average,
        "sequential_std": sequential_std,
        "threads": threads,
        "parallel_times": parallel_times,
        "parallel_std": parallel_std,
        "speedups": speedups,
        "efficiencies": efficiencies
    }


def plot_execution_time(data, image_size, output_dir):
    threads = data["threads"]

    sequential_time = data["sequential_time"] * 1000

    parallel_times = [
        value * 1000
        for value in data["parallel_times"]
    ]

    parallel_std = [
        value * 1000
        for value in data["parallel_std"]
    ]

    plt.figure(figsize=(9, 5))

    plt.axhline(
        y=sequential_time,
        linestyle="--",
        label="Sequential"
    )

    plt.errorbar(
        threads,
        parallel_times,
        yerr=parallel_std,
        marker="o",
        capsize=5,
        linewidth=2,
        label="Parallel"
    )

    plt.xlabel("Number of Threads")
    plt.ylabel("Execution Time (ms)")
    plt.title("Grayscale Execution Time")

    plt.xticks(threads)
    plt.grid(True, alpha=0.3)
    plt.legend()

    plt.tight_layout()

    filename = output_dir / f"execution_time_{image_size}.png"
    plt.savefig(filename, dpi=200)
    plt.close()


def plot_speedup(data, image_size, output_dir):
    threads = data["threads"]
    speedups = data["speedups"]

    plt.figure(figsize=(9, 5))

    plt.plot(
        threads,
        speedups,
        marker="o",
        linewidth=2,
        label="Actual Speedup"
    )

    plt.plot(
        threads,
        threads,
        linestyle="--",
        label="Ideal Speedup"
    )

    plt.xlabel("Number of Threads")
    plt.ylabel("Speedup")
    plt.title("Grayscale Parallel Speedup")

    plt.xticks(threads)
    plt.grid(True, alpha=0.3)
    plt.legend()

    plt.tight_layout()

    filename = output_dir / f"speedup_{image_size}.png"
    plt.savefig(filename, dpi=200)
    plt.close()


def plot_efficiency(data, image_size, output_dir):
    threads = data["threads"]
    efficiencies = data["efficiencies"]

    plt.figure(figsize=(9, 5))

    plt.plot(
        threads,
        efficiencies,
        marker="o",
        linewidth=2,
        label="Parallel Efficiency"
    )

    plt.axhline(
        y=100,
        linestyle="--",
        label="Ideal Efficiency"
    )

    plt.xlabel("Number of Threads")
    plt.ylabel("Efficiency (%)")
    plt.title("Grayscale Parallel Efficiency")

    plt.xticks(threads)
    plt.grid(True, alpha=0.3)
    plt.legend()

    plt.tight_layout()

    filename = output_dir / f"efficiency_{image_size}.png"
    plt.savefig(filename, dpi=200)
    plt.close()


def generate_charts(results, output_dir):
    output_dir.mkdir(parents=True, exist_ok=True)

    groups = group_results(results)

    for image, records in groups.items():
        width, height, channels = image

        image_size = f"{width}x{height}"

        data = calculate_statistics(records)

        plot_execution_time(data, image_size, output_dir)
        plot_speedup(data, image_size, output_dir)
        plot_efficiency(data, image_size, output_dir)

        correct = all(
            record["outputs_identical"]
            for record in records
        )

        print(f"\nImage: {image_size}")
        print(f"Benchmark runs: {len(records)}")
        print(f"Correctness: {'PASS' if correct else 'FAIL'}")
        print(f"Sequential: {data['sequential_time'] * 1000:.4f} ms")

        for i, threads in enumerate(data["threads"]):
            print(
                f"{threads} threads: "
                f"{data['parallel_times'][i] * 1000:.4f} ms | "
                f"Speedup: {data['speedups'][i]:.2f}x | "
                f"Efficiency: {data['efficiencies'][i]:.2f}%"
            )

    print(f"\nCharts saved to: {output_dir}")


def main():
    parser = argparse.ArgumentParser(
        description="Generate charts from image processing benchmarks."
    )

    parser.add_argument(
        "--input",
        default="results/benchmark_results.jsonl",
        help="Path to benchmark results file."
    )

    parser.add_argument(
        "--output",
        default="results/charts",
        help="Directory for generated charts."
    )

    args = parser.parse_args()

    results = load_results(args.input)

    if not results:
        print("No benchmark results found!")
        return

    generate_charts(results, Path(args.output))


if __name__ == "__main__":
    main()
