
AM_CPPFLAGS = -I${top_srcdir} -I${top_builddir}

this_includedir=${includedir}/${subdir}
this_include_HEADERS = \
    all.hpp \
    blackconstantvol.hpp \
    blackvariancecurve.hpp \
    blackvariancesurface.hpp \
    blackvoltermstructure.hpp \
    fixedlocalvolsurface.hpp \
    gridmodellocalvolsurface.hpp \
    hestonblackvolsurface.hpp \
    impliedvoltermstructure.hpp \
    localconstantvol.hpp \
    localvolcurve.hpp \
    localvolsurface.hpp \
    localvoltermstructure.hpp \
    noexceptlocalvolsurface.hpp

libEquityFxVol_la_SOURCES = \
    blackvariancecurve.cpp \
    blackvariancesurface.cpp \
    blackvoltermstructure.cpp \
    fixedlocalvolsurface.cpp \
    gridmodellocalvolsurface.cpp \
    hestonblackvolsurface.cpp \
    localvolsurface.cpp \
    localvoltermstructure.cpp

noinst_LTLIBRARIES = libEquityFxVol.la

all.hpp: Makefile.am
	echo "/* This file is automatically generated; do not edit.     */" > $@
	echo "/* Add the files to be included into Makefile.am instead. */" >> $@
	echo >> $@
	for i in $(filter-out all.hpp, $(this_include_HEADERS)); do \
		echo "#include <${subdir}/$$i>" >> $@; \
	done
	echo >> $@
	subdirs='$(SUBDIRS)'; for i in $$subdirs; do \
		echo "#include <${subdir}/$$i/all.hpp>" >> $@; \
	done

