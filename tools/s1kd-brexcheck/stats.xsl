<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

  <xsl:output method="text"/>
 
  <xsl:template match="brexCheck">
    <xsl:text>Layered check: </xsl:text>
    <xsl:value-of select="@layered"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:text>Check object values: </xsl:text>
    <xsl:value-of select="@checkObjectValues"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:text>SNS check: </xsl:text>
    <xsl:value-of select="@snsCheck"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:text>Notation check: </xsl:text>
    <xsl:value-of select="@notationCheck"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:variable name="total" select="count(document)"/>
    <xsl:text>Total documents checked: </xsl:text>
    <xsl:value-of select="$total"/>
    <xsl:text>&#10;</xsl:text>
    <xsl:if test="$total &gt; 0">
      <xsl:variable name="errors" select="count(//error/object|//error[not(object)])"/>
      <xsl:variable name="xpath-errors" select="count(//xpathError)"/>
      <xsl:variable name="fail" select="count(document[brex/error[@fail != 'no']])"/>
      <xsl:variable name="pass" select="count(document[not(brex/error) or brex/error/@fail = 'no'])"/>
      <xsl:text>Total BREX errors: </xsl:text>
      <xsl:value-of select="$errors"/>
      <xsl:text>&#10;</xsl:text>
      <xsl:if test="$xpath-errors &gt; 0">
        <xsl:text>XPath errors: </xsl:text>
        <xsl:value-of select="$xpath-errors"/>
        <xsl:text>&#10;</xsl:text>
      </xsl:if>
      <xsl:text>Total documents that pass the check: </xsl:text>
      <xsl:value-of select="$pass"/>
      <xsl:text>&#10;</xsl:text>
      <xsl:text>Total documents that fail the check: </xsl:text>
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
