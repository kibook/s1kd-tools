<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="acronymDefinition" mode="id">
    <xsl:choose>
      <xsl:when test="@keepId">
        <xsl:value-of select="@id"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>acr-</xsl:text>
        <xsl:number count="acronym" from="dmodule" level="any" format="0001"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="acronymDefinition/@id">
    <xsl:attribute name="id">
      <xsl:apply-templates select=".." mode="id"/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="acronymDefinition/@keepId"/>

  <xsl:template match="acronymTerm/@internalRefId">
    <xsl:variable name="ref" select="."/>
    <xsl:variable name="def" select="//acronymDefinition[@id = $ref]"/>
    <xsl:attribute name="internalRefId">
      <xsl:choose>
        <xsl:when test="$def">
          <xsl:apply-templates select="$def" mode="id"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$ref"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
  </xsl:template>

</xsl:stylesheet>
