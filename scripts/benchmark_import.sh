#!/usr/bin/env bash

set -euo pipefail

readonly RUNS="${RUNS:-5}"
readonly EXPECTED_VARIANTS="${EXPECTED_VARIANTS:-2459837}"

readonly EXECUTABLE="${EXECUTABLE:?Set EXECUTABLE to the vcf_importer executable}"
readonly VCF_FILE="${VCF_FILE:?Set VCF_FILE to the input VCF path}"
readonly VCF_DATABASE_PATH="${VCF_DATABASE_PATH:?Set VCF_DATABASE_PATH to the SQLite database path}"
readonly OUTPUT_DIR="${OUTPUT_DIR:-benchmark-results}"

mkdir -p "$OUTPUT_DIR"

remove_database()
{
    rm -f \
        "$VCF_DATABASE_PATH" \
        "$VCF_DATABASE_PATH-wal" \
        "$VCF_DATABASE_PATH-shm" \
        "$VCF_DATABASE_PATH-journal"
}

validate_import()
{
    local row_count

    row_count="$(
        sqlite3 "$VCF_DATABASE_PATH" \
            "SELECT COUNT(*) FROM variants;"
    )"

    if [[ "$row_count" != "$EXPECTED_VARIANTS" ]]; then
        echo "Validation failed: expected $EXPECTED_VARIANTS rows, found $row_count" >&2
        exit 1
    fi
}

echo "===== Warm-up run ====="

remove_database

"$EXECUTABLE" --vcf "$VCF_FILE" >/dev/null

validate_import

echo "Warm-up complete"

echo "run,elapsed_seconds,max_rss_kb" > "$OUTPUT_DIR/results.csv"

for run in $(seq 1 "$RUNS"); do
    echo
    echo "===== Run $run/$RUNS ====="

    remove_database

    timing_file="$OUTPUT_DIR/timing-$run.txt"
    output_file="$OUTPUT_DIR/output-$run.txt"

    /usr/bin/time \
        -f "elapsed_seconds=%e\nmax_rss_kb=%M" \
        -o "$timing_file" \
        "$EXECUTABLE" --vcf "$VCF_FILE" \
        2>&1 | tee "$output_file"

    validate_import

    elapsed_seconds="$(awk -F= '/elapsed_seconds/ {print $2}' "$timing_file")"
    max_rss_kb="$(awk -F= '/max_rss_kb/ {print $2}' "$timing_file")"

    echo "$run,$elapsed_seconds,$max_rss_kb" >> "$OUTPUT_DIR/results.csv"

    echo "Validated $EXPECTED_VARIANTS rows"
    echo "Elapsed: ${elapsed_seconds}s"
    echo "Peak RSS: ${max_rss_kb} KB"
done

echo
echo "===== Results ====="
column -s, -t "$OUTPUT_DIR/results.csv" 2>/dev/null \
    || cat "$OUTPUT_DIR/results.csv"

echo
echo "Raw outputs and timing data are stored in: $OUTPUT_DIR"
