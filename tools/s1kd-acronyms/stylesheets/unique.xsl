<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="acronyms">
    <xsl:copy>
      <xsl:apply-templates select="acronym">
        <xsl:sort select="acronymTerm"/>
        <xsl:sort select="@acronymType"/>
        <xsl:sort select="acronymDefinition"/>
      </xsl:apply-templates>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="acronym">
    <xsl:copy>
      <xsl:apply-templates select="@*|*"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="acronym[
    acronymTerm = preceding::acronymTerm and
    acronymDefinition = preceding::acronymDefinition and
    (not(@acronymType) or @acronymType=preceding::acronym/@acronymType)
  ]"/>

</xsl:stylesheet>
