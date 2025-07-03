#!/bin/bash -ex

verbose=0
git_verbose=0
max_jobs=16

lockfile="$(pwd)/version_check.lock"
resultfile="$(pwd)/version_check_results.txt"

if [ "$1" = "-v" ]; then
    verbose=1
elif [ "$1" = "-vv" ]; then
    verbose=1
    git_verbose=1
fi

# Limit concurrent jobs
job_control()
{
    while [ "$(jobs | wc -l)" -ge "$max_jobs" ]; do
        sleep 0.1
    done
}

git_call()
{
    if [ "$git_verbose" -eq 1 ]; then
        eval "git $@"
    elif [[ "$1" == "fetch" ]]; then
        eval "git $@ >/dev/null 2>&1"
    else
        eval "git $@ 2>/dev/null"
    fi
}

check_tags()
{
    local path=$(echo "$1" | sed 's:/*$::')

    echo "Checking $path"

    git_call fetch --prune --all

    # Get the latest tag (by commit date, not tag name)
    local tag_list=$(git_call rev-list --tags --max-count=1)
    local latest_tag=$(git_call describe --tags "$tag_list")
    local highest_tag=$(git_call tag -l | sort -V | tail -n1)

    if [ -n "$latest_tag" ] || [ -n "$highest_tag" ]; then

        # Get the current tag
        local current_tag=$(git_call describe --tags --abbrev=0)

        if [ -n "$current_tag" ]; then

            if [ "$verbose" -eq 1 ]; then
                echo "$path -> latest: $latest_tag, highest: $highest_tag, current: $current_tag"
            fi

            local ts0=$(git_call log -1 --format=%ct $highest_tag)
            local ts1=$(git_call log -1 --format=%ct $latest_tag)
            local ts2=$(git_call log -1 --format=%ct $current_tag)

            if (( ts0 > ts2 )) || (( ts1 > ts2 )); then
                if [ "$verbose" -eq 1 ]; then
                    echo -e "\t $path: latest is newer"
                elif [ "$verbose" -eq 0 ]; then
                    echo "$path -> latest: $latest_tag, highest: $highest_tag, current: $current_tag"
                fi

                # Critical section guarded by flock
                (
                    flock 200
                    echo "$path -> latest: $latest_tag, highest: $highest_tag, current: $current_tag" >> "$resultfile"
                ) 200>"$lockfile"
            fi

        elif [ "$verbose" -eq 1 ]; then
            echo "$path -> latest: $latest_tag, highest: $highest_tag"
        fi
    elif [ "$verbose" -eq 1 ]; then

        if [ -n "$current_tag" ]; then
            echo "$path -> current: $current_tag"
        else
            echo "$path -> no tags found"
        fi
    fi
}

# Fetch and check repositories multi threaded
for submoduledir in */ ;
do
    cd "$submoduledir" || continue

    if [ -e ".git" ]; then
        job_control
        check_tags "$submoduledir" &
    else
        for sub in */ ;
        do
            if [ -e "$sub/.git" ]; then
                cd "$sub" || continue
                job_control
                check_tags "$submoduledir$sub" &
                cd .. || exit
            fi
        done
    fi

    cd .. || exit
done

# Wait for all background jobs to finish
wait

# Print results
echo -e "\n\nResult:\n"

# Find the max length of the paths (before '->')
max_len=0
while IFS='->' read -r left _; do
    len=$(echo -n "$left" | wc -c)
    if (( len > max_len )); then
        max_len=$len
    fi
done < "$resultfile"

# Print with padding so '->' lines up
while IFS='->' read -r left right; do
    right=$(echo "$right" | sed 's/^[[:space:]]*>*[[:space:]]*//')
    printf "%-${max_len}s -> %s\n" "$left" "$right"
done < "$resultfile"

# Remove tmp files
rm -f "$resultfile"
rm -f "$lockfile"
