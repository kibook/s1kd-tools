<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="acronym">
    <xsl:variable name="term" select="acronymTerm"/>
    <xsl:variable name="defn" select="acronymDefinition"/>
    <xsl:variable name="type" select="@acronymType"/>
    <xsl:variable name="prec" select="preceding::acronym[
      acronymTerm = $term and
      acronymDefinition = $defn and
      (@acronymType = $type or not(@acronymType or $type))]"/>
    <xsl:choose>
      <xsl:when test="$prec">
        <acronymTerm>
          <xsl:attribute name="internalRefId">
            <xsl:value-of select="generate-id($prec[1])"/>
          </xsl:attribute>
          <xsl:apply-templates select="acronymTerm/@*|acronymTerm/node()"/>
        </acronymTerm>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy>
          <xsl:attribute name="id">
            <xsl:value-of select="generate-id()"/>
          </xsl:attribute>
          <xsl:apply-templates select="@*|node()"/>
        </xsl:copy>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
