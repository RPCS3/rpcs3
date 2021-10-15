#!/bin/sh -e

# Export variables for later stages of the Cirrus pipeline
# Values done in this manner will appear as environment variables
# in later stages.

# From pure-sh-bible
# Setting 'IFS' tells 'read' where to split the string.
while IFS='=' read -r key val; do
    # Skip over lines containing comments.
    [ "${key##\#*}" ] || continue
    export "$key"="$val"
done < ".ci/azure-vars.env"
