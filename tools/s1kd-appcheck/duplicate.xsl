<?xml version="1.0" encoding="UTF-8"?>
<xsl:transform
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:s1kd-appcheck="urn:s1kd-tools:s1kd-appcheck"
  version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="applic">
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="s1kd-appcheck:string">
        <xsl:apply-templates select="assert|evaluate|expression" mode="string"/>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="assert" mode="string">
    <xsl:value-of select="@applicPropertyIdent|@actidref"/>
    <xsl:text>:</xsl:text>
    <xsl:value-of select="@applicPropertyType|@actreftype"/>
    <xsl:text>=</xsl:text>
    <xsl:value-of select="@applicPropertyValues|@actvalues"/>
  </xsl:template>

  <xsl:template match="evaluate" mode="string">
    <xsl:variable name="op" select="@andOr|@operator"/>
    <xsl:text>(</xsl:text>
    <xsl:value-of select="$op"/>
    <xsl:text> </xsl:text>
    <xsl:for-each select="expression">
      <xsl:apply-templates select="." mode="string"/>
      <xsl:if test="position() != last()">
        <xsl:text> </xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:for-each select="evaluate">
      <xsl:apply-templates select="." mode="string"/>
      <xsl:if test="position() != last()">
        <xsl:text> </xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:for-each select="assert">
      <xsl:apply-templates select="." mode="string"/>
      <xsl:if test="position() != last()">
        <xsl:text> </xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:text>)</xsl:text>
  </xsl:template>

</xsl:transform>
