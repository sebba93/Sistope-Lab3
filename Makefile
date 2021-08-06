proceso_principal = lab3

principal_headers = funciones.h
principal_source  = $(principal_headers:.h=.c) lab3.c
principal_objects = $(principal_source:.c=.o) 

CC     = gcc
CFLAGS = -Wall -lm

depends = .depends

build : $(proceso_principal) 

$(proceso_principal) : $(principal_objects)
	$(CC) $(CFLAGS) -o $@ $^ -lm

$(objects):
	$(CC) $(CFLAGS) -c -o $@ $*.c

$(depends) : $(principal_source) $(principal_headers)
	@$(CC) -MM $(principal_source) > $@


clean :
	$(RM) $(proceso_principal) $(principal_objects) $(zipfile) $(depends)

.PHONY : build zip clean

sinclude $(depends)
