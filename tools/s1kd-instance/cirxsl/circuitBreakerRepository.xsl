<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>
  
  <xsl:template match="circuitBreakerRef">
    <xsl:variable name="circuitBreakerNumber" select="@circuitBreakerNumber"/>
    <xsl:variable name="circuitBreakerIdent" select="(//circuitBreakerIdent[@circuitBreakerNumber = $circuitBreakerNumber])[1]"/>
    <xsl:variable name="circuitBreakerSpec" select="$circuitBreakerIdent/parent::circuitBreakerSpec"/>
    <circuitBreakerRef>
      <xsl:apply-templates select="@*"/>
      <xsl:choose>
        <xsl:when test="$circuitBreakerSpec">
          <xsl:apply-templates select="$circuitBreakerSpec/name"/>
          <xsl:apply-templates select="$circuitBreakerSpec/shortName"/>
          <xsl:apply-templates select="$circuitBreakerSpec/refs"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="name|shortName|refs"/>
        </xsl:otherwise>
      </xsl:choose>
    </circuitBreakerRef>
  </xsl:template>

</xsl:stylesheet>
