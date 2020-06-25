<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="brexMap">
    <xsl:element name="xsl:stylesheet">
      <xsl:attribute name="version">1.0</xsl:attribute>
      <xsl:element name="xsl:template">
        <xsl:attribute name="match">/</xsl:attribute>
        <xsl:element name="defaults">
          <xsl:element name="xsl:apply-templates">
            <xsl:attribute name="select">//structureObjectRule</xsl:attribute>
          </xsl:element>
        </xsl:element>
      </xsl:element>
      <xsl:element name="xsl:template">
        <xsl:attribute name="match">structureObjectRule</xsl:attribute>
        <xsl:element name="xsl:variable">
          <xsl:attribute name="name">ident</xsl:attribute>
          <xsl:element name="xsl:choose">
            <xsl:apply-templates select="default"/>
          </xsl:element>
        </xsl:element>
        <xsl:element name="xsl:if">
          <xsl:attribute name="test">$ident != '' and objectValue</xsl:attribute>
          <xsl:element name="default">
            <xsl:attribute name="ident">{$ident}</xsl:attribute>
            <xsl:attribute name="value">{objectValue[1]/@valueAllowed}</xsl:attribute>
          </xsl:element>
        </xsl:element>
      </xsl:element>
    </xsl:element>
  </xsl:template>

  <xsl:template match="default">
    <xsl:element name="xsl:when">
      <xsl:attribute name="test">
        <xsl:choose>
          <xsl:when test="@id">
            <xsl:text>@id = '</xsl:text>
            <xsl:value-of select="@id"/>
            <xsl:text>'</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>objectPath = '</xsl:text>
            <xsl:value-of select="@path"/>
            <xsl:text>'</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
      <xsl:value-of select="@ident"/>
    </xsl:element>
  </xsl:template>

</xsl:stylesheet>
