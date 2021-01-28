#!/bin/sh

die() {
	if [ $# -ge 1 ]; then
		echo "$@"
	else
		cat -
	fi
	exit 1
}

if git rev-parse --verify HEAD >/dev/null 2>&1
then
	against=HEAD
else
	# Initial commit: diff against an empty tree object
	against=$(git hash-object -t tree /dev/null)
fi

# prevent non-ASCII names
if [ $(git diff --cached --name-only --diff-filter=ACR -z $against |
	LC_ALL=C tr -d '[ -~]\0' |
	wc -c) != 0 ]
then
	die <<EOF
Error: Attempt to add a non-ASCII file name.

This can cause problems if you want to work with people on other platforms.

Or people with a different keyboard. Or people who don't hate themselves enough.

Don't do it, c'mon.
EOF
fi

# avoid random spaces and diff markers
# (although the space should be taken care of by the next hook)
if ! git -c "core.whitespace=-blank-at-eof" diff-index --check --cached $against -- ; then
	die "Error: introducing diff markers or spurious whitespace."
fi

# ensure formatting
tempfile=$(mktmp)
repo=$(git rev-parse --show-toplevel)
git diff --cached --name-only --diff-filter=ACM | while read fname; do
	case "$fname" in
		"*.c"|"*.h")
				current="${repo}/${fname}"
				clang-format "$current" > "$tempfile"
				if ! diff "$current" "$tempfile"; then
					die "Error: unstable formatting"
				fi
			;;
		*) ;;
	esac
done

