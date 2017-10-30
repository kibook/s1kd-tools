<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="acronymDefinition" mode="id">
    <xsl:text>acr-</xsl:text>
    <xsl:number count="acronym" from="dmodule" level="any" format="0001"/>
  </xsl:template>

  <xsl:template match="acronymDefinition/@id">
    <xsl:attribute name="id">
      <xsl:apply-templates select=".." mode="id"/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="acronymTerm/@internalRefId">
    <xsl:variable name="ref" select="."/>
    <xsl:attribute name="internalRefId">
      <xsl:apply-templates select="//acronymDefinition[@id = $ref]" mode="id"/>
    </xsl:attribute>
  </xsl:template>

</xsl:stylesheet>
