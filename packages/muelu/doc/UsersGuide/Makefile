#

#TEX = latex
TEX = pdflatex

BTEX = bibtex

BIBFILE = mueluguide.bib

PAPER = mueluguide

PARTS = mueluguide.tex

PICTURES =

SIAM = siam.bst       siam10.clo     siamuelutex.cls   siamuelutex.sty docultex.tex lexample.tex

all: ${PAPER}.pdf

########################################################################
#
# Dependencies - 3 are required for latex/bibtex
# - a rule must exist for the .dvi, the .bbl and the .aux file
#   in order for a clean make with a new .tex file
#
########################################################################

${PAPER}.dvi: ${PAPER}.tex ${PARTS} #${PAPER}.bbl
	${TEX} ${PAPER}
	${TEX} ${PAPER}

${PAPER}.bbl: ${PAPER}.aux
	${BTEX} ${PAPER}

${PAPER}.aux: ${PAPER}.tex ${PARTS} force
	${TEX} ${PAPER}

force:

${PAPER}.ps:    ${PAPER}.dvi
	dvips -Ppdf -o ${PAPER}.ps ${PAPER}

${PAPER}.pdf:    ${PAPER}.dvi

tarfile:
	tar chvf ${PAPER}.tar Makefile ${PARTS} ${PICTURES} ${BIBFILE} ${SIAM}
	compress ${PAPER}.tar

clean:
	/bin/rm -f ${PAPER}.pdf ${PAPER}.ps ${PAPER}.dj ${PAPER}.dvi *.aux *.bbl *.blg *.log *.lof *.lot *.toc *.tar *.tar.Z ${PAPER}.out
