#!/bin/sh -e

# Export variables for later stages of the Azure pipeline
# Values done in this manner will appear as environment variables
# in later stages.

# From pure-sh-bible
# Setting 'IFS' tells 'read' where to split the string.
while IFS='=' read -r key val; do
    # Skip over lines containing comments.
    [ "${key##\#*}" ] || continue
    echo "##vso[task.setvariable variable=$key]$val"
done < ".ci/azure-vars.env"
