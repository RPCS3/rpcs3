#!/bin/sh -ex

verbose=0
git_verbose=0

if [ "$1" = "-v" ]; then
    verbose=1
elif [ "$1" = "-vv" ]; then
    verbose=1
    git_verbose=1
fi

max_dir_length=0
result_dirs=()
result_msgs=()

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
    path=$(echo "$1" | sed 's:/*$::')

    echo "Checking $path"

    git_call fetch --prune --all

    # Get the latest tag (by commit date, not tag name)
    tag_list=$(git_call rev-list --tags --max-count=1)
    latest_tag=$(git_call describe --tags "$tag_list")

    if [ -n "$latest_tag" ]; then

        # Get the current tag
        current_tag=$(git_call describe --tags --abbrev=0)

        if [ -n "$current_tag" ]; then

            if [ "$verbose" -eq 1 ]; then
                echo "$path -> latest: $latest_tag, current: $current_tag"
            fi

            ts1=$(git_call log -1 --format=%ct $latest_tag)
            ts2=$(git_call log -1 --format=%ct $current_tag)

            if (( ts1 > ts2 )); then
                if [ "$verbose" -eq 1 ]; then
                    echo -e "\t $path: latest is newer"
                elif [ "$verbose" -eq 0 ]; then
                    echo "$path -> latest: $latest_tag, current: $current_tag"
                fi

                path_length=${#path}
                if (( $path_length > $max_dir_length )); then
                    max_dir_length=$path_length
                fi
                result_dirs+=("$path")
                result_msgs+=("latest: $latest_tag, current: $current_tag")
            fi

        elif [ "$verbose" -eq 1 ]; then
            echo "$path -> latest: $latest_tag"
        fi
    elif [ "$verbose" -eq 1 ]; then

        if [ -n "$current_tag" ]; then
            echo "$path -> current: $current_tag"
        else
            echo "$path -> no tags found"
        fi
    fi
}

for submoduledir in */ ;
do
    cd "$submoduledir" || continue

    if [ -e ".git" ]; then
        check_tags "$submoduledir"
    else
        # find */ -mindepth 1 -maxdepth 1 -type d | while read -r sub;
        for sub in */ ;
        do
            if [ -e "$sub/.git" ]; then
                cd "$sub" || continue
                check_tags "$submoduledir$sub"
                cd .. || exit
            fi
        done
    fi

    cd .. || exit
done

echo -e "\n\nResult:\n"
i=0
for result_dir in "${result_dirs[@]}"; do
    msg=""
    diff=$(($max_dir_length - ${#result_dir}))
    if (( $diff > 0 )); then
        msg+=$(printf "%${diff}s" "")
    fi
    msg+="$result_dir"
    echo "$msg -> ${result_msgs[$i]}"
    ((i++))
done
echo ""
