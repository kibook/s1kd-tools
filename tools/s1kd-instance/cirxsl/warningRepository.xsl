<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="warningsAndCautionsRef">
    <warningsAndCautions>
      <xsl:apply-templates select="@*|node()"/>
    </warningsAndCautions>
  </xsl:template>

  <xsl:template match="warningRef">
    <xsl:variable name="warningIdentNumber" select="@warningIdentNumber"/>
    <xsl:variable name="warningIdent" select="//warningIdent[$warningIdentNumber = @warningIdentNumber]"/>
    <xsl:variable name="warningSpec" select="$warningIdent/parent::warningSpec"/>
    <xsl:choose>
      <xsl:when test="$warningSpec">
        <warning>
          <xsl:apply-templates select="@id"/>
          <xsl:apply-templates select="$warningSpec/warningAndCautionPara"/>
        </warning>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy>
          <xsl:apply-templates select="@*|node()"/>
        </xsl:copy>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
