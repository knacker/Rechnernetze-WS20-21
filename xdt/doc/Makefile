# Makefile fo build xdt API documentation

CONF	= doxygen.conf
DOC	= html
BACKUP	= $(DOC).backup

SCONF	= doxygen-static.conf
SDOC	= html-static
SBACKUP	= $(SDOC).backup


## PAPER = desc
## FIGURES = stack.pdf example.pdf model.pdf


###

.PHONY: all dox nonstatic static clean

all: dox
dox: nonstatic static
## desc: $(PAPER).pdf


nonstatic:
	# backup old docs
	test -d $(DOC) && mv $(DOC) $(BACKUP) || true
	# build documentation
	doxygen $(CONF)
	# copy footer images, remove backup on success
	test -d $(DOC) && cp btu-logo.png rnks-logo.png $(DOC) && rm -rf $(BACKUP) || true

static:
	# backup old docs
	test -d $(SDOC) && mv $(SDOC) $(SBACKUP) || true
	# build documentation
	doxygen $(SCONF)
	# copy footer images, remove backup on success
	test -d $(SDOC) && cp btu-logo.png rnks-logo.png $(SDOC) && rm -rf $(SBACKUP) || true


##$(PAPER).pdf: $(PAPER).tex $(FIGURES)
##	pdflatex $(PAPER) && pdflatex $(PAPER)

##%.pdf : %.fig
##	fig2dev -L pdf $< $@

##%.pdf : %.eps
##	epstopdf $< > $@


clean:
	rm -rf $(DOC) $(BACKUP) $(SDOC) $(SBACKUP) *~
