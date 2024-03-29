<?xml version="1.0"?>
<xsl:transform
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="s1kd_instance_acronym">
    <xsl:variable name="internalRefId" select="@internalRefId"/>
    <xsl:choose>
      <xsl:when test="preceding::s1kd_instance_acronym[@internalRefId = $internalRefId]">
        <acronymTerm internalRefId="{$internalRefId}">
          <xsl:value-of select="acronym/acronymTerm"/>
        </acronymTerm>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy-of select="acronym"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:transform>
