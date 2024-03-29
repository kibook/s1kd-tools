<?xml version="1.0"?>
<xsl:transform
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="acronym">
    <s1kd_instance_acronym>
      <xsl:attribute name="internalRefId">
        <xsl:choose>
          <xsl:when test="acronymDefinition/@id and //@internalRefId = acronymDefinition/@id">
            <xsl:value-of select="acronymDefinition/@id"/>
          </xsl:when>
          <xsl:when test="@id and //@internalRefId = @id">
            <xsl:value-of select="@id"/>
          </xsl:when>
          <xsl:when test="acronymDefinition/@id">
            <xsl:value-of select="@acronymDefinition/@id"/>
          </xsl:when>
          <xsl:when test="@id">
            <xsl:value-of select="@id"/>
          </xsl:when>
        </xsl:choose>
      </xsl:attribute>
      <xsl:copy-of select="."/>
    </s1kd_instance_acronym>
  </xsl:template>

  <xsl:template match="acronymTerm[not(parent::acronym)]">
    <xsl:variable name="internalRefId" select="@internalRefId"/>
    <s1kd_instance_acronym internalRefId="{$internalRefId}">
      <xsl:copy-of select="(//acronym[@id = $internalRefId or acronymDefinition/@id = $internalRefId])[1]"/>
    </s1kd_instance_acronym>
  </xsl:template>

</xsl:transform>
