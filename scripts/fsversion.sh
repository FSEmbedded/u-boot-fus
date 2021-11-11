#!/bin/sh
#

gitdiff=`git diff-index --name-only HEAD`
gitsha=`git rev-parse HEAD`
gittag=`git describe --tags --abbrev=0`
gittagsha=`git rev-parse ${gittag}`

if [[ -z "${gitdiff}" ]]
then
	# Git is clean
	if [[ "${gitsha}" == "${gittagsha}" ]]
	then
		# Gittag is the newest version
		echo "${gittag}"
	else
		# Gittag is outdated
		echo "${gitsha}"
	fi
else
	# Git is dirty
	echo "${gitsha}-dirty"
fi
