for f in FIFO.c popen1.c popen2.c potok1.c potok2.c potok3.c task.c; do 
	gcc -Wall -Wextra -std=c11 -O2 "$f" -o "${f%.c}" 
done
