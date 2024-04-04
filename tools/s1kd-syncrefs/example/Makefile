source=DMC-SYNCREFS-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
output=DMC-SYNCREFS-A-00-00-00-00A-040A-D_000-02_EN-CA.XML

all: $(output)

$(output): $(source)
	s1kd-upissue $(source)
	s1kd-syncrefs -f $(output)
	xmllint --format $(output) -o $(output)

clean:
	$(RM) $(output)
