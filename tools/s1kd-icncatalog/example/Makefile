src = DMC-TEST-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
out = DMC-TEST-A-00-00-00-00A-040A-D_000-01_EN-CA.pdf

icncatalog = ../s1kd-icncatalog

$(out): $(src) .icncatalog
	$(icncatalog) -m pdf $(src) | s1kd2pdf -o $(out) -
