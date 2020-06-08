OUTPUTS+=all.txt
OUTPUTS+=abbreviations.xml
OUTPUTS+=terms.xml
OUTPUTS+=definitionList.xml
OUTPUTS+=DMC-ACRONYMS-A-00-00-00-00A-005A-D_000-02_EN-CA.XML
OUTPUTS+=DMC-ACRONYMS-A-00-00-02-00A-000A-D_000-02_EN-CA.XML

all: $(OUTPUTS)

DMC-ACRONYMS-A-00-00-02-00A-000A-D_000-02_EN-CA.XML: DMC-ACRONYMS-A-00-00-02-00A-000A-D_000-01_EN-CA.XML abbreviations.xml
	s1kd-upissue DMC-ACRONYMS-A-00-00-02-00A-000A-D_000-01_EN-CA.XML
	s1kd-acronyms -m abbreviations.xml DMC-ACRONYMS-A-00-00-02-00A-000A-D_000-02_EN-CA.XML

all.txt: DMC-*.XML
	s1kd-acronyms -p DMC-*.XML > all.txt

abbreviations.xml: DMC-*.XML
	s1kd-acronyms -xpT at01 DMC-*.XML > abbreviations.xml

terms.xml: DMC-*.XML
	s1kd-acronyms -xpT at02 DMC-*.XML > terms.xml

definitionList.xml: DMC-*.XML
	s1kd-acronyms -xpdT at01 DMC-*.XML > definitionList.xml

DMC-ACRONYMS-A-00-00-00-00A-005A-D_000-02_EN-CA.XML: DMC-ACRONYMS-A-00-00-00-00A-005A-D_000-01_EN-CA.XML definitionList.xml
	s1kd-upissue DMC-ACRONYMS-A-00-00-00-00A-005A-D_000-01_EN-CA.XML
	xml-merge DMC-ACRONYMS-A-00-00-00-00A-005A-D_000-02_EN-CA.XML definitionList.xml | xmllint --format - | sponge DMC-ACRONYMS-A-00-00-00-00A-005A-D_000-02_EN-CA.XML

clean:
	rm -f $(OUTPUTS)
