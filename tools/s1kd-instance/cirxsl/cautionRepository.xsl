<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="warningsAndCautionsRef">
    <warningsAndCautions>
      <xsl:apply-templates select="@*|node()"/>
    </warningsAndCautions>
  </xsl:template>

  <xsl:template match="cautionRef">
    <xsl:variable name="cautionIdentNumber" select="@cautionIdentNumber"/>
    <xsl:variable name="cautionIdent" select="(//cautionIdent[$cautionIdentNumber = @cautionIdentNumber])[1]"/>
    <xsl:variable name="cautionSpec" select="$cautionIdent/parent::cautionSpec"/>
    <xsl:choose>
      <xsl:when test="$cautionSpec">
        <caution>
          <xsl:apply-templates select="@id"/>
          <xsl:apply-templates select="$cautionSpec/warningAndCautionPara"/>
        </caution>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy>
          <xsl:apply-templates select="@*|node()"/>
        </xsl:copy>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
