<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  exclude-result-prefixes="xsi">

  <xsl:variable name="schema-prefix">http://www.s1000d.org/S1000D_4-2/xml_schema_flat/</xsl:variable>
  
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="@xsi:noNamespaceSchemaLocation">
    <xsl:attribute name="xsi:noNamespaceSchemaLocation">
      <xsl:text>http://www.s1000d.org/S1000D_4-1/xml_schema_flat/</xsl:text>
      <xsl:value-of select="substring-after(., $schema-prefix)"/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="dmTitle">
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:apply-templates select="techName|infoName"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="infoName">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
      <xsl:apply-templates select="parent::dmTitle/infoNameVariant"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="infoNameVariant">
    <xsl:text>, </xsl:text>
    <xsl:apply-templates select="node()"/>
  </xsl:template>

</xsl:stylesheet>
