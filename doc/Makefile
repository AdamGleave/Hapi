LATEXMK = latexmk

.PHONY: prop progress diss

all: prop progress diss
prop: proposal.pdf
progress: progressreport.pdf
diss: diss.pdf

proposal.pdf: proposal.tex propbody.tex diss.bib
	latexmk -bibtex -pdf -pdflatex="pdflatex -interactive=nonstopmode -use-make proposal.tex" proposal.tex

progressreport.pdf: progressreport.tex progressbody.tex diss.bib
	latexmk -bibtex -pdf -pdflatex="pdflatex -interactive=nonstopmode -use-make progressreport.tex" progressreport.tex

diss.pdf: diss.tex diss.bib
	scripts/texcount.pl -inc diss.tex | grep -A5 "File(s) total" | sed -n 2p | sed -e 's/Words in text: //' > diss-auto-wordcount.tex
	latexmk -bibtex -pdf -pdflatex="pdflatex -interactive=nonstopmode -use-make diss.tex" diss.tex

wordcount:
	scripts/texcount.pl -inc diss.tex | grep -A5 "File(s) total" | sed -n 2p | sed -e 's/Words in text: //'

clean:
	rm -f *.dvi *.aux *.log *.err
	rm -f *.pdf *.out
	rm -f core *~ *.lof *.toc *.blg *.bbl
	rm -f *.fdb_latexmk

help:
	@echo
	@echo "USAGE:"
	@echo
	@echo "make              display help information"
	@echo "make prop         make the proposal"
	@echo "make progress     make the progress report"
	@echo "make diss         make the dissertation"
	@echo "make all          make the proposal and dissertation"
	@echo "make wordcount    display an estimated word count"
	@echo "make clean        remove all remakeable files"
	@echo

