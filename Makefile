release:
	gcc -Os -o glyphgen glyphgen.c -lncursesw
	strip -s -R .comment glyphgen

debug:
	gcc -g -o glyphgen glyphgen.c -lncursesw

clean:
	rm -f glyphgen
