#!/bin/sh

ln -s "../../.githooks/pre-commit.sh" "$(git rev-parse --show-toplevel)/.git/hooks/pre-commit"

