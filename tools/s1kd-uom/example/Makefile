.PHONY: all clean

S1KD_UOM=../s1kd-uom
SOURCE=DMC-EX-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
TO_MARKDOWN=s1kd2db - | pandoc -f docbook -t markdown_github

# Fetch exchange rates from the internet
CAD2EUR=\$$value * `sh exchange-rate.sh CAD EUR`
CAD2USD=\$$value * `sh exchange-rate.sh CAD USD`

all: README.md

# Build a README out of the concatenated output tables
README.md: output-CA.md output-EU.md output-US.md
	sh concat.sh $+ > $@

# Deploy for a Canadian customer by:
# - Pre-formatting quantities with the imperial conventions
output-CA.md: $(SOURCE)
	$(S1KD_UOM) -p imperial $< | $(TO_MARKDOWN) > $@

# Deploy for an EU customer by:
# - Converting Canadian dollars to euros at the current exchange rate, with the format 0.00
# - Pre-formatting quantities with the SI conventions
output-EU.md: $(SOURCE)
	$(S1KD_UOM) -u CAD -t EUR -e "$(CAD2EUR)" -F '0.00' -p SI $< | $(TO_MARKDOWN) > $@

# Deploy for a US customer by:
# - Converting SI units to US customary units (-s US), rounding to the nearest whole number (-F '0')
# - Converting Canadian dollars to US dollars at the current exchange rate, with the format 0.00
# - Pre-formatting quantities with the imperial conventions
output-US.md: $(SOURCE)
	$(S1KD_UOM) -F '0' -s US -u CAD -t USD -e "$(CAD2USD)" -F '0.00' -p imperial $< | $(TO_MARKDOWN) > $@

clean:
	rm -f output-CA.md output-EU.md output-US.md README.md
