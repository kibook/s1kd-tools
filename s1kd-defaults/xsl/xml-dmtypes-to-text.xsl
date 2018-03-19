<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- Convert XML s1kd-newdm 'dmtypes' file to text. -->

  <xsl:output method="text"/>

  <xsl:template match="*">
    <xsl:apply-templates select="*|comment()"/>
  </xsl:template>

  <xsl:template match="type">
    <xsl:value-of select="@infoCode"/>
    <xsl:text>&#09;</xsl:text>
    <xsl:value-of select="@schema"/>
    <xsl:if test="@infoName">
      <xsl:text>&#09;</xsl:text>
      <xsl:value-of select="@infoName"/>
    </xsl:if>
    <xsl:text>&#10;</xsl:text>
  </xsl:template>

  <xsl:template match="comment()">
    <xsl:text>#</xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>&#10;</xsl:text>
  </xsl:template>

</xsl:stylesheet>
