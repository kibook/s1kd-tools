<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

  <xsl:output method="text"/>
 
  <xsl:template match="appCheck">
    <xsl:text>Type of check: </xsl:text>
    <xsl:value-of select="@type"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:text>Strict: </xsl:text>
    <xsl:value-of select="@strict"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:text>Check nested applicability: </xsl:text>
    <xsl:value-of select="@checkNestedApplic"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:variable name="total" select="count(object)"/>
    <xsl:text>Total objects checked: </xsl:text>
    <xsl:value-of select="$total"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:if test="$total &gt; 0">
      <xsl:variable name="fail" select="count(object[@valid = 'no'])"/>
      <xsl:variable name="pass" select="count(object[@valid = 'yes'])"/>
      <xsl:text>Total objects that pass the check: </xsl:text>
      <xsl:value-of select="$pass"/>
      <xsl:text>&#10;</xsl:text>
      <xsl:text>Total objects that fail the check: </xsl:text>
      <xsl:value-of select="$fail"/>
      <xsl:text>&#10;</xsl:text>
      <xsl:text>Percentage passed: </xsl:text>
      <xsl:value-of select="floor($pass div $total * 100)"/>
      <xsl:text>%&#10;</xsl:text>
      <xsl:text>Percentage failed: </xsl:text>
      <xsl:value-of select="ceiling($fail div $total * 100)"/>
      <xsl:text>%&#10;</xsl:text>
    </xsl:if>
  </xsl:template>

</xsl:stylesheet>
