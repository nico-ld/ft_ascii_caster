#!/bin/bash

# Check first argument
if [[ -z "$1" ]]; then
    echo -e "\033[31m\033[1m[ERROR] :\033[0m\033[31m No argument given.\033[0m"
    exit 1
fi

# Check too many arguments
if [[ -n "$2" ]]; then
    echo -e "\033[31m\033[1m[ERROR] :\033[0m\033[31m Too many arguments given.\033[0m"
    exit 1
fi

# Check directory exists
if [[ ! -d "$1" ]]; then
    echo -e "\033[31m\033[1m[ERROR] :\033[0m\033[31m No such directory named $1.\033[0m"
    exit 1
fi

# Check executable exists
if [[ ! -x "./ft_ascii_caster" ]]; then
    echo "The tester must be placed in the same directory as the executable 'so_long'"
    exit 1
fi

# Loop through .ber files
for file in "$1"/*.map; do
    if [[ -f "$file" ]]; then
        echo -e "Testing '$file'"
        valgrind -q ./ft_ascii_caster "$file"
        echo -e "\n"
    fi
done