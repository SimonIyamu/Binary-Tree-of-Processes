all: myfind searcher splittermerger

myfind : root.o
	gcc root.o -o myfind -g -lm

searcher: searcher.o
	gcc searcher.o -o searcher -g

splittermerger: splittermerger.o
	gcc splittermerger.o -o splittermerger -g -lm

root.o : root.c dataTypes.h
	gcc -c root.c -g

searcher.o : searcher.c dataTypes.h
	gcc -c searcher.c -g

splittermerger.o: splittermerger.c dataTypes.h
	gcc -c splittermerger.c -g

clean:
	rm myfind root.o searcher searcher.o splittermerger splittermerger.o
