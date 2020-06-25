<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="brexMap">
    <xsl:element name="xsl:stylesheet">
      <xsl:attribute name="version">1.0</xsl:attribute>
      <xsl:element name="xsl:template">
        <xsl:attribute name="match">/</xsl:attribute>
        <xsl:element name="dmtypes">
          <xsl:apply-templates select="dmtypes"/>
        </xsl:element>
      </xsl:element>
      <xsl:element name="xsl:template">
        <xsl:attribute name="match">structureObjectRule</xsl:attribute>
        <xsl:element name="xsl:apply-templates">
          <xsl:attribute name="select">objectValue</xsl:attribute>
        </xsl:element>
      </xsl:element>
      <xsl:element name="xsl:template">
        <xsl:attribute name="match">objectValue</xsl:attribute>
        <xsl:element name="type">
          <xsl:attribute name="infoCode">{@valueAllowed}</xsl:attribute>
          <xsl:attribute name="infoName">{.}</xsl:attribute>
        </xsl:element>
      </xsl:element>
    </xsl:element>
  </xsl:template>

  <xsl:template match="dmtypes">
    <xsl:element name="xsl:apply-templates">
      <xsl:attribute name="select">
        <xsl:text>//structureObjectRule[</xsl:text>
        <xsl:choose>
          <xsl:when test="@id">
            <xsl:text>@id = '</xsl:text>
            <xsl:value-of select="@id"/>
            <xsl:text>'</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>objectPath ='</xsl:text>
            <xsl:value-of select="@path"/>
            <xsl:text>'</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:text>]</xsl:text>
      </xsl:attribute>
    </xsl:element>
  </xsl:template>

</xsl:stylesheet>
