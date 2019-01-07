<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

  <xsl:output method="text"/>
 
  <xsl:template match="brexCheck">
    <xsl:variable name="total" select="count(document)"/>
    <xsl:variable name="fail" select="count(document[brex/error[@fail != 'no']])"/>
    <xsl:variable name="pass" select="count(document[not(brex/error) or brex/error/@fail = 'no'])"/>
    <xsl:text>Total documents checked: </xsl:text>
    <xsl:value-of select="count(document)"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:text>Total BREX errors: </xsl:text>
    <xsl:value-of select="count(//error)"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:text>Total documents that pass the check: </xsl:text>
    <xsl:value-of select="$pass"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:text>Total documents that fail the check: </xsl:text>
    <xsl:value-of select="$fail"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:text>Percentage passed: </xsl:text>
    <xsl:value-of select="format-number($pass div $total * 100, '0.##')"/>
    <xsl:text>%&#10;</xsl:text>
    <xsl:text>Percentage failed: </xsl:text>
    <xsl:value-of select="format-number($fail div $total * 100, '0.##')"/>
    <xsl:text>%&#10;</xsl:text>
  </xsl:template>

</xsl:stylesheet>
