<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>
  
  <xsl:template match="controlIndicatorRef">
    <xsl:variable name="controlIndicatorNumber" select="@controlIndicatorNumber"/>
    <xsl:variable name="controlIndicatorSpec" select="(//controlIndicatorSpec[@controlIndicatorNumber = $controlIndicatorNumber])[1]"/>
    <controlIndicatorRef>
      <xsl:apply-templates select="@*"/>
      <xsl:choose>
        <xsl:when test="$controlIndicatorSpec">
          <xsl:apply-templates select="$controlIndicatorSpec/controlIndicatorName" mode="name"/>
          <xsl:apply-templates select="$controlIndicatorSpec/shortName"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="name|shortName"/>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:apply-templates select="refs"/>
    </controlIndicatorRef>
  </xsl:template>

  <xsl:template match="controlIndicatorName" mode="name">
    <name>
      <xsl:apply-templates select="@*|node()"/>
    </name>
  </xsl:template>

</xsl:stylesheet>
