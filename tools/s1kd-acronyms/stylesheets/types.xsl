<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  
  <xsl:param name="types"/>

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="acronyms">
    <xsl:copy>
      <xsl:apply-templates select="*"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="acronym">
    <xsl:if test="not($types) or (@acronymType and contains($types, @acronymType))">
      <xsl:copy>
        <xsl:apply-templates select="@*|*"/>
      </xsl:copy>
    </xsl:if>
  </xsl:template>


</xsl:stylesheet>
