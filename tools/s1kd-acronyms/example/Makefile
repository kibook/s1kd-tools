outputs += all.txt
outputs += abbreviations.xml
outputs += terms.xml
outputs += definitionList.xml
outputs += DMC-ACRONYMS-A-00-00-00-00A-005A-D_000-02_EN-CA.XML
outputs += DMC-ACRONYMS-A-00-00-02-00A-000A-D_000-02_EN-CA.XML
outputs += DMC-ACRONYMS-A-00-00-03-00A-000A-D_000-02_EN-CA.XML

all: $(outputs)

DMC-ACRONYMS-A-00-00-02-00A-000A-D_000-02_EN-CA.XML: DMC-ACRONYMS-A-00-00-02-00A-000A-D_000-01_EN-CA.XML abbreviations.xml
	s1kd-upissue DMC-ACRONYMS-A-00-00-02-00A-000A-D_000-01_EN-CA.XML
	s1kd-acronyms -M abbreviations.xml -f DMC-ACRONYMS-A-00-00-02-00A-000A-D_000-02_EN-CA.XML

DMC-ACRONYMS-A-00-00-03-00A-000A-D_000-02_EN-CA.XML: DMC-ACRONYMS-A-00-00-03-00A-000A-D_000-01_EN-CA.XML DMC-ACRONYMS-A-00-00-00-00A-0A7A-D_000-01_EN-US.XML
	s1kd-upissue DMC-ACRONYMS-A-00-00-03-00A-000A-D_000-01_EN-CA.XML
	s1kd-acronyms -M DMC-ACRONYMS-A-00-00-00-00A-0A7A-D_000-01_EN-US.XML -f DMC-ACRONYMS-A-00-00-03-00A-000A-D_000-02_EN-CA.XML

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
	rm -f $(outputs)
