BUILT_SOURCES=parser.h
AM_YFLAGS=-d
bin_PROGRAMS=dcc
dcc_SOURCES=dcc.cpp ast.cpp symbol_table.cpp parser.ypp scanner.lpp adjoint_code_joint.cpp tangent_linear_code.cpp ast.hpp symbol_table.hpp adjoint_code_joint.hpp tangent_linear_code.hpp 
dcc_LDADD=-lfl -lm

SUBDIRS=tests/test1 

EXTRA_DIST= tests/test1/f.c \
tests/test1/reference_output/a1_f.c tests/test1/reference_output/t1_f.c 

check-local :
	cd tests/test1 && ../../dcc f.c 1 1 \
                       && diff t1_f.c reference_output/t1_f.c \
	               && ../../dcc f.c 2 1 \
                       && diff a1_f.c reference_output/a1_f.c
	
