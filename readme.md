# VCF Importer

## Overview

A C++20 command-line tool that reads a VCF file and stores its variants in SQLite.

The importer uses a streaming parser to process variants incrementally, persists data through a data-access layer, serializes selected VCF fields into JSON, and creates an index on chromosome and position after the bulk import.

## Features

- Streaming VCF parsing without loading the entire file into memory.
- Parsing of VCF columns, INFO fields, FORMAT fields, and samples.
- INFO and FORMAT values are converted using their types declared in the VCF header.
- SQLite persistence through a dedicated repository.
- JSON serialization of FILTER, QUAL, INFO, and FORMAT.
- Uses a transaction and a reusable prepared statement for bulk imports
- Chromosome-position index created after the bulk import.
- Unit tests using GoogleTest.

## Architecture

The application is divided into four main areas:

### CLI

Validates command-line arguments, loads configuration, creates the application components, and controls the import flow.

### VCF parser

`VcfParser` owns the input stream and parsed header. It processes variants one at a time through `readNextVariant()`, avoiding loading the complete file into memory.

### Domain model

Domain types represent variants, filters, INFO entries, FORMAT entries, samples, and VCF header definitions.

### Persistence

`Database` owns the SQLite connection.

`DatabaseTransaction` owns the transaction lifetime. It begins a transaction on construction, commits explicitly, and rolls back automatically if the operation exits without committing.

`VariantRepository` creates the schema, owns the reusable insert statement, serializes variant data, inserts rows, and creates the chromosome-position index.

```text
CLI
 ├── VcfParser
 ├── Database
 ├── VariantRepository
 └── DatabaseTransaction

VCF file
   ↓
VcfParser::readNextVariant()
   ↓
Variant
   ↓
VariantRepository::insert()
   ↓
SQLite
```

## Project Structure

```text
src/
├── cli/
├── config/
├── domain/
├── persistence/
└── vcf/

tests/
├── config/
├── data/
├── parser/
└── persistence/
```

## Requirements

- C++20-compatible compiler
- CMake
- SQLite3 development package
- Git

## Third-Party Libraries

- SQLite3 — database storage
- nlohmann/json — JSON serialization
- GoogleTest — unit testing

SQLite3 is provided by the system and located through CMake using `find_package`.

nlohmann/json and GoogleTest are downloaded automatically during CMake configuration using `FetchContent`.

```bash
sudo apt update
sudo apt install build-essential cmake libsqlite3-dev
```

## Build

```bash
git clone [<repository-url>](https://github.com/omar2h/vcf-importer.git)
cd vcf-importer

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The executable is created at `build/src/vcf_importer`

## Configuration

The SQLite database path is configured through the `VCF_DATABASE_PATH` environment variable.

```bash
export VCF_DATABASE_PATH=/tmp/vcf_importer.db
```

## Usage

Run the importer with the required `--vcf` argument:

```bash
./build/src/vcf_importer --vcf /path/to/input.vcf
```

Example using the provided assignment VCF file:

```bash
export VCF_DATABASE_PATH=/tmp/vcf_importer.db

./build/src/vcf_importer --vcf /path/to/assignment.final.vcf
```

Successful output:

```text
Imported variants: 2459837
```

## Database Schema

```sql
CREATE TABLE IF NOT EXISTS variants
(
    id INTEGER PRIMARY KEY,
    chromosome TEXT NOT NULL,
    position INTEGER NOT NULL,
    ref TEXT NOT NULL,
    alt TEXT NOT NULL,
    data TEXT NOT NULL
);
```

The index is created after the bulk insert:

```sql
CREATE INDEX IF NOT EXISTS idx_variants_chromosome_position
ON variants(chromosome, position);
```

## JSON Representation

The `data` column stores the remaining selected variant fields:

```json
{
  "FILTER": "PASS",
  "QUAL": 42.5,
  "INFO": {
    "DP": 14,
    "AF": 0.5,
    "DB": true
  },
  "FORMAT": {
    "GT": "0/1",
    "DP": 8
  }
}
```

Header definitions are used to infer whether INFO and FORMAT values are integers, floating-point values, strings, characters, or flags.

## Sorting Behavior

Variants are queried in chromosome-position order using:

```sql
SELECT chromosome, position, ref, alt, data
FROM variants
ORDER BY chromosome, position;
```

The `idx_variants_chromosome_position` index supports this query efficiently.

The index is created after the bulk insert to avoid maintaining it for every inserted row.

Chromosome values are kept as text because VCF contig names are not always numeric. This supports values such as `chrX`, `chrM`, and `GL000207.1`.
Because the database column is text, chromosome ordering is lexical. For example, `1`, `10`, and `2` are returned in that order.

## Testing

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The test suite covers:

* VCF header and record parsing
* malformed input handling
* streaming iteration and end-of-file behavior
* environment configuration
* SQLite connection and transaction behavior
* schema and index creation
* prepared-statement reuse
* JSON serialization

## Performance

Benchmarks were performed using:

- Ubuntu 24.04 under WSL2
- Release build
- input containing 2,459,837 variant records
- input and database stored on the WSL Linux filesystem
- a fresh SQLite database for every run

| Benchmark | Median time | Observed range | Peak RSS |
|---|---:|---:|---:|
| Complete SQLite import, including index creation | 91.8 s | 86.7–99.3 s | approximately 9 MB |

Each measured import was validated by checking that the database contained
2,459,837 rows.

### Running the benchmark

Script is provided at `scripts/benchmark_import.sh`.

The script performs one warm-up import followed by the configured number of
runs. Before each run, it removes the existing SQLite database and
associated journal files. It records elapsed wall-clock time and peak resident
memory, then verifies the imported row count.

The following environment variables are required:

| Variable | Description |
|---|---|
| `EXECUTABLE` | Path to the `vcf_importer` executable |
| `VCF_FILE` | Path to the input VCF file |
| `VCF_DATABASE_PATH` | Path to the SQLite database used by the benchmark |

Run the benchmark from the repository root:

```bash
EXECUTABLE=./build/src/vcf_importer \
VCF_FILE=/path/to/assignment.final.vcf \
VCF_DATABASE_PATH=/tmp/vcf_importer.db \
./scripts/benchmark_import.sh
```

The following optional environment variables can override the default benchmark
configuration:

| Variable | Default | Description |
|---|---:|---|
| `RUNS` | `5` | Number of runs |
| `EXPECTED_VARIANTS` | `2459837` | Expected database row count after each import |
| `OUTPUT_DIR` | `benchmark-results` | Directory in which benchmark results are stored |

Example with custom values:

```bash
EXECUTABLE=./build/src/vcf_importer \
VCF_FILE=/path/to/assignment.final.vcf \
VCF_DATABASE_PATH=/tmp/vcf_importer.db \
RUNS=5 \
EXPECTED_VARIANTS=2459837 \
OUTPUT_DIR=benchmark-results \
./scripts/benchmark_import.sh
```

The output directory contains:

- `results.csv`, containing the elapsed time and peak RSS for every run
- per-run timing files
- per-run application output

## References

- [VCF 4.3 Specification](https://samtools.github.io/hts-specs/VCFv4.3.pdf) — parsing behavior and field interpretation
- [SQLite Documentation](https://www.sqlite.org/docs.html) — database behavior and SQL features
- [SQLite C/C++ Interface](https://www.sqlite.org/cintro.html) — connection, prepared-statement, and resource lifecycle
