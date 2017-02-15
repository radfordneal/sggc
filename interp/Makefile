all:	interp interp-db interp-db-offset interp-set-static \
	interp-set-max-static interp-set-max-static-offset \
	interp-noseg interp-nobuiltins

interp:	interp.c sggc.c set.c sggc-app.h sggc.h set-app.h set.h
	gcc -g -O3 -march=native -mtune=native interp.c sggc.c set.c -o interp

interp-db:	interp.c sggc.c set.c sggc-app.h sggc.h set-app.h set.h
	gcc -g -O0 -DSET_DEBUG=1 \
	 interp.c sggc.c set.c -o interp-db

interp-db-offset:	interp.c sggc.c set.c sggc-app.h sggc.h set-app.h set.h
	gcc -g -O0 -DSET_DEBUG=1 -DSGGC_USE_OFFSET_POINTERS=1 \
	 interp.c sggc.c set.c -o interp-db-offset

interp-set-static:	interp.c sggc.c set.c sggc-app.h sggc.h set-app.h set.h
	gcc -g -O3 -march=native -mtune=native \
	  -DSET_STATIC=1 interp.c sggc.c -o interp-set-static

interp-set-max-static:	interp.c sggc.c set.c sggc-app.h sggc.h set-app.h set.h
	gcc -g -O3 -march=native -mtune=native \
	  -DSGGC_MAX_SEGMENTS=10000 -DSET_STATIC=1 \
	  interp.c sggc.c -o interp-set-max-static

interp-set-max-static-offset:	interp.c sggc.c set.c sggc-app.h sggc.h \
				set-app.h set.h
	gcc -g -O3 -march=native -mtune=native \
	 -DSGGC_MAX_SEGMENTS=10000 -DSET_STATIC=1 -DSGGC_USE_OFFSET_POINTERS=1 \
	 interp.c sggc.c -o interp-set-max-static-offset

interp-noseg:	interp.c sggc.c set.c sggc-app.h sggc.h \
		set-app.h set.h
	gcc -g -O3 -march=native -mtune=native \
	 -DSGGC_MAX_SEGMENTS=10000 -DSET_STATIC=1 -DSGGC_USE_OFFSET_POINTERS=1 \
	 -DSGGC_SEGMENT_AT_A_TIME=0 interp.c sggc.c -o interp-noseg

interp-nobuiltins:	interp.c sggc.c set.c sggc-app.h sggc.h \
			set-app.h set.h
	gcc -g -O3 -march=native -mtune=native \
	 -DSGGC_MAX_SEGMENTS=10000 -DSET_STATIC=1 -DSGGC_USE_OFFSET_POINTERS=1 \
	 -DSET_USE_BUILTINS=0 interp.c sggc.c -o interp-nobuiltins
