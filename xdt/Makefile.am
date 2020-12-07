AUTOMAKE_OPTIONS = foreign #dist-bzip2

EXTRA_DIST = README FLAVOUR doc scripts
SUBDIRS = src

# exclude CVS files and generated docs from distribution, indent source
dist-hook:
	rm -rf $(distdir)/doc/CVS $(distdir)/scripts/CVS
	(cd $(distdir)/doc && make clean)


dox:
	(cd $(top_srcdir)/doc && make dox)

##desc:
##	(cd $(top_srcdir)/doc && make desc)


info-files:
	$(top_srcdir)/scripts/make-flavour info

imt-files:
	$(top_srcdir)/scripts/make-flavour imt

