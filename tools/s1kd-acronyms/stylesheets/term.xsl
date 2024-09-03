<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
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
            <xsl:choose>
              <xsl:when test="$prec[1]/acronymDefinition/@id">
                <xsl:value-of select="$prec[1]/acronymDefinition/@id"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="generate-id($prec[1])"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:attribute>
          <xsl:apply-templates select="acronymTerm/@*|acronymTerm/node()"/>
        </acronymTerm>
      </xsl:when>
      <xsl:otherwise>
        <acronym>
          <xsl:apply-templates select="@*"/>
          <xsl:apply-templates select="acronymTerm"/>
          <acronymDefinition>
            <xsl:choose>
              <!-- If the acronymDefinition has an ID in the .acronyms file,
                   keep that one. The @keepId attribute tells the next
                   stylesheet not to override it. -->
              <xsl:when test="acronymDefinition/@id">
                <xsl:attribute name="id">
                  <xsl:value-of select="acronymDefinition/@id"/>
                </xsl:attribute>
                <xsl:attribute name="keepId"/>
              </xsl:when>
              <!-- If the acronymDefinition in the .acronyms file has no ID,
                   autogenerate one, which will be replaced with an ID in the
                   form of acr-xxxx in the next stylesheet. -->
              <xsl:otherwise>
                <xsl:attribute name="id">
                  <xsl:value-of select="generate-id()"/>
                </xsl:attribute>
              </xsl:otherwise>
            </xsl:choose>
            <xsl:apply-templates select="acronymDefinition/node()"/>
          </acronymDefinition>
        </acronym>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="ignoredAcronym">
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="terminologySpec">
    <terminologyRef>
      <xsl:apply-templates select="terminologyIdent/@terminologyIdentNumber"/>
    </terminologyRef>
  </xsl:template>

</xsl:stylesheet>
