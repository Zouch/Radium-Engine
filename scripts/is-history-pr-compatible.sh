#!/bin/bash
# dlyr 2020, based on
# https://stackoverflow.com/questions/26983700/git-run-shell-command-for-each-commit
# https://stackoverflow.com/questions/3824050/telling-if-a-git-commit-is-a-merge-revert-commit

gitismerge () {
    local sha=$1
    msha=$(git rev-list -1 --merges ${sha}~1..${sha})
    [ -z "$msha" ] && return 1
    echo "merge commit"
    return 0
}

if [ `git rev-parse origin/release-candidate` != `git merge-base origin/release-candidate HEAD` ]; then
    echo "Please rebase your branch with \"rebase origin/master\" ";
    exit 1
else
    ko=""
    while read -r rev; do
	echo "check " `git log -1 --pretty=oneline  --abbrev-commit  ${rev}`
	ok=`gitismerge $rev`
	if [[ -n $ok ]]; then
	    echo " ${rev} is a merge commit, please rebase and remove (e.g. squash) all merge commits";
	    ko="1"
	fi
    done< <(git rev-list origin/release-candidate..HEAD)
    if [[ -z $ko ]]; then
	echo "congrats your branch history is compatible with our PR guidelines";
	exit 0
    else
	exit 1
    fi
fi
