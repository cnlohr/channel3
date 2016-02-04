all : synthtables MakeCbTables

synthtables : synthtables.c
	gcc -o $@ $^ -g -lm
	./synthtables

MakeCbTables : MakeCbTables.c
	gcc -o $@ $^ -g -lm
	./MakeCbTables

clean :
	rm -rf synthtables MakeCbTables CbTable.h CbTable.c broadcast_tables.c broadcast_tables.h
