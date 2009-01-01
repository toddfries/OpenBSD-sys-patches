if [ "$(git branch | egrep "^\* freebsd")" ]; then
	while ! cvs -d $(cat CVS/Root) -q up -PAd
	do
		echo "    FAIL : $(date) $d"
		sleep $((5+RANDOM%600)).$((RANDOM%100))
		echo "    START: $(date) $d"
	done
	echo "    SUCCESS: $(date) $d\n"
	gitsync && git commit -m sync -a && git repack && git gc && git push
else
	echo "not on the master branch, must 'git checkout master' first"
fi
