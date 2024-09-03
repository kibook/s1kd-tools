<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="terminologyRef">
    <xsl:variable name="terminologyIdentNumber" select="@terminologyIdentNumber"/>
    <xsl:variable name="terminologySpec" select="(//terminologySpec[terminologyIdent[@terminologyIdentNumber = $terminologyIdentNumber]])[1]"/>
    <xsl:choose>
      <xsl:when test="$terminologySpec">
        <acronym>
          <xsl:apply-templates select="$terminologySpec/terminologyTerm" mode="acronym"/>
          <xsl:apply-templates select="$terminologySpec/terminologyDefinition" mode="acronym"/>
        </acronym>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy>
          <xsl:apply-templates select="@*|node()"/>
        </xsl:copy>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="terminologyTerm" mode="acronym">
    <acronymTerm>
      <xsl:apply-templates/>
    </acronymTerm>
  </xsl:template>

  <xsl:template match="terminologyDefinition" mode="acronym">
    <acronymDefinition>
      <xsl:apply-templates select="terminologyPara[1]" mode="acronym"/>
    </acronymDefinition>
  </xsl:template>

  <xsl:template match="terminologyPara" mode="acronym">
    <xsl:apply-templates/>
  </xsl:template>

</xsl:stylesheet>
