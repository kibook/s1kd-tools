<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="@acronymType">
    <xsl:attribute name="acrotype">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="@internalRefId">
    <xsl:attribute name="xrefid">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="acronymTerm">
    <acroterm>
      <xsl:apply-templates select="@*|node()"/>
    </acroterm>
  </xsl:template>

  <xsl:template match="acronymDefinition">
    <acrodef>
      <xsl:apply-templates select="@*|node()"/>
    </acrodef>
  </xsl:template>

</xsl:stylesheet>
