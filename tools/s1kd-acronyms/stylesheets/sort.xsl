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
        <xsl:sort select="string-length(acronymTerm)" order="descending"/>
      </xsl:apply-templates>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="terminologyRepository">
    <xsl:copy>
      <xsl:apply-templates select="terminologySpec">
        <xsl:sort select="string-length(terminologyTerm)" order="descending"/>
      </xsl:apply-templates>
    </xsl:copy>
  </xsl:template>

</xsl:stylesheet>
