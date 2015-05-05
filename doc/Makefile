LATEXMK = latexmk

.PHONY: prop progress diss diss_continual

all: prop progress diss
prop: proposal.pdf
progress: progressreport.pdf
diss: diss.pdf
diff: diff.pdf

proposal.pdf: proposal.tex propbody.tex diss.bib
	latexmk -bibtex -pdf -pdflatex="pdflatex -interactive=nonstopmode -use-make proposal.tex" proposal.tex

progressreport.pdf: progressreport.tex progressbody.tex diss.bib
	latexmk -bibtex -pdf -pdflatex="pdflatex -interactive=nonstopmode -use-make progressreport.tex" progressreport.tex

diss.pdf: diss.tex diss-intro.tex diss-prep.tex diss-impl.tex diss-eval.tex diss-conc.tex diss.bib
	@echo "Fetching figures (you may need to enter srguser password)"
	scripts/caelum_fetch_figures.sh
	@echo "Compiling dissertation"
	scripts/texcount.pl -inc diss.tex | grep -A5 "File(s) total" | sed -n 2p | sed -e 's/Words in text: //' > diss-auto-wordcount.tex
	latexmk -bibtex -pdf -pdflatex="pdflatex -interactive=nonstopmode -use-make diss.tex" diss.tex

diff.pdf: diff.tex
	latexmk -bibtex -pdf -pdflatex="pdflatex -interactive=nonstopmode -use-make diff.tex" diff.tex

diss_continual: diss.tex diss.bib
	latexmk -pvc -bibtex -pdf -pdflatex="pdflatex -interactive=nonstopmode -use-make diss.tex" diss.tex

wordcount:
	scripts/texcount.pl -inc diss.tex | grep -A5 "File(s) total" | sed -n 2p | sed -e 's/Words in text: //' | tee diss-auto-wordcount.tex

clean:
	rm -f *.pdf
	latexmk -c

help:
	@echo
	@echo "USAGE:"
	@echo
	@echo "make              display help information"
	@echo "make prop         make the proposal"
	@echo "make progress     make the progress report"
	@echo "make diss         make the dissertation"
	@echo "make diff         only used by latexdiff"
	@echo "make all          make the proposal and dissertation"
	@echo "make wordcount    display an estimated word count"
	@echo "make clean        remove all remakeable files"
	@echo

