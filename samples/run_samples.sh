#!/usr/bin/env bash

set -e

program="$(find ../build/ -type f \( -iname "contractor" -o -iname "contractor.exe" \) )"

if [[ ! -f "$program" ]]; then
	echo "Unable to find the executable at $program"
	exit 1
fi

for dir in $(find . -maxdepth 1 -mindepth 1 -type d); do
	dir_name="$(basename "$dir")"

	for export_file in $(find "./$dir_name" -maxdepth 1 -mindepth 1 -type f -iname "*.EXPORT"); do
		out_file="${dir_name}_$(basename $export_file .EXPORT).out"
		symmetry_file="$(find ./$dir_name -type f -iname "*.symmetry")"
		symmetry_file="$(find ./$dir_name -type f -iname "*.symmetry")"

		echo "Processing '$export_file' - output goes to '$out_file' ..."

		"$program" --decomposition "./density_fitting.decomposition" --symmetry "$symmetry_file" --index-spaces \
			"./index_spaces.json" --gecco-export "$export_file" --restricted-orbitals > "$out_file"
	done
done
