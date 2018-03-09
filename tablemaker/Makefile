all : MakeCbTables

MakeCbTables : MakeCbTables.c
	gcc -o $@ $^ -g -lm
	./MakeCbTables

clean :
	rm -rf synthtables MakeCbTables CbTable.h CbTable.c broadcast_tables.c broadcast_tables.h
