SRC=DMC-TEST-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
OUT=DMC-TEST-A-00-00-00-00A-040A-D_000-01_EN-CA.pdf

ICNCATALOG=../s1kd-icncatalog

$(OUT): $(SRC) .icncatalog
	$(ICNCATALOG) -m pdf $(SRC) | s1kd2pdf -o $(OUT) -
