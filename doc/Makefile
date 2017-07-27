SOURCE=DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_000-01_EN-CA.XML

../README.md: $(SOURCE)
	s1kd2db $(SOURCE) | pandoc -f docbook -t markdown_github -s -o ../README.md
