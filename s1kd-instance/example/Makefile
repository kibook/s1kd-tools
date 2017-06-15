SRC=DMC-S1000DBIKE-AAA-D00-00-00-00AA-258A-A_009-00_en-US.XML

MK9=DME-B6865-MK9-S1000DBIKE-AAA-D00-00-00-00AA-258A-A_009-00_en-US.XML
MK1=DME-B6865-MK1-S1000DBIKE-AAA-D00-00-00-00AA-258A-A_009-00_en-US.XML

all: $(MK9) $(MK1)

$(MK9): $(SRC)
	s1kd-instance -s $(SRC) -e B6865-MK9 -O . \
		"type:prodattr=Mountain bicycle" \
		"model:prodattr=Brook trekker" \
		"version:prodattr=Mk9"

$(MK1): $(SRC)
	s1kd-instance -s $(SRC) -e B6865-MK1 -O . \
		"type:prodattr=Mountain bicycle" \
		"model:prodattr=Mountain storm" \
		"version:prodattr=Mk1"

clean:
	rm -f $(MK9) $(MK1)
