#!/usr/bin/env bash

set -euo pipefail

dirs=("src" "scheme")

total=0

for dir in "${dirs[@]}"; do
  if [[ -d "$dir" ]]; then
    count=$(find "$dir" -type f -print0 \
      | xargs -0 cat \
      | wc -l)
    echo "$dir: $count"
    total=$((total + count))
  else
    echo "$dir: (not found)"
  fi
done

echo "Total LoC: $total"
