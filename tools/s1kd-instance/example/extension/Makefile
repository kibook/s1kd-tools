src = DMC-S1000DBIKE-AAA-D00-00-00-00AA-258A-A_009-00_en-US.XML

mk9 = DME-B6865-MK9-S1000DBIKE-AAA-D00-00-00-00AA-258A-A_009-00_EN-US.XML
mk1 = DME-B6865-MK1-S1000DBIKE-AAA-D00-00-00-00AA-258A-A_009-00_EN-US.XML

all: $(mk9) $(mk1)

$(mk9): $(src)
	s1kd-instance -e B6865-MK9 -O . \
		-s "type:prodattr=Mountain bicycle" \
		-s "model:prodattr=Brook trekker" \
		-s "version:prodattr=Mk9" \
		$(src)

$(mk1): $(src)
	s1kd-instance -e B6865-MK1 -O . \
		-s "type:prodattr=Mountain bicycle" \
		-s "model:prodattr=Mountain storm" \
		-s "version:prodattr=Mk1" \
		$(src)

clean:
	$(RM) $(mk9) $(mk1)
