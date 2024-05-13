#!/bin/bash
for file_path in "$@"; do
    if [ ! -f "$file_path" ]; then
        echo "1"
        continue
    fi
    if [ ! -r "$file_path" ]; then
        echo "2"
        continue
    fi
    if [ ! -w "$file_path" ]; then
        echo "3"
        continue
    fi
    if grep -q "virus" "$file_path"; then
        echo "4"
    else
        echo "0"
    fi
done
