<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="hazardRef">
    <xsl:variable name="hazardIdentNumber" select="@hazardIdentNumber"/>
    <xsl:variable name="hazardSpec" select="(//hazardSpec[hazardIdent[@hazardIdentNumber = $hazardIdentNumber]])[1]"/>
    <xsl:choose>
      <xsl:when test="$hazardSpec">
        <hazard>
          <xsl:apply-templates select="@id"/>
          <xsl:apply-templates select="$hazardSpec/symbol"/>
          <xsl:apply-templates select="$hazardSpec/hazardousClass"/>
          <xsl:apply-templates select="$hazardSpec/safetyInformation"/>
        </hazard>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy>
          <xsl:apply-templates select="@*|node()"/>
        </xsl:copy>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
