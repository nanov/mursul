#!/bin/bash

SOLUTION=$(curl -s https://www.nytimes.com/svc/wordle/v2/2024-05-21.json | jq -r ."solution")
./bin/mursul $SOLUTION
