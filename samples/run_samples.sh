#!/usr/bin/env bash

set -e

mainDir="$(dirname "$(readlink -f "$0")")"
program="$mainDir/../build/bin/contractor"

if [ ! -f "$program" ]; then
	echo "Unable to find the executable at $program"
	exit 1
fi

for dir in $(find "$mainDir" -maxdepth 1 -mindepth 1 -type d); do
	dir_name="$(basename "$dir")"

	for export_file in $(find "$mainDir/$dir_name" -maxdepth 1 -mindepth 1 -type f -iname "*.EXPORT"); do
		out_file="${dir_name}_$(basename $export_file .EXPORT).out"
		symmetry_file="$(find $mainDir/$dir_name -type f -iname "*.symmetry")"
		symmetry_file="$(find $mainDir/$dir_name -type f -iname "*.symmetry")"

		echo "Processing '$export_file' - output goes to '$out_file' ..."

		"$program" --decomposition "$mainDir/density_fitting.decomposition" --symmetry "$symmetry_file" --index-spaces \
			"$mainDir/index_spaces.json" --gecco-export "$export_file" --restricted-orbitals > "$out_file"
	done
done
