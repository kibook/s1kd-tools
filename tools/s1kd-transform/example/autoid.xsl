<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- Give levelledPara's automatic incrementing IDs -->

  <xsl:template match="levelledPara">
    <xsl:copy>
      <xsl:attribute name="id">
        <xsl:text>par-</xsl:text>
        <xsl:number format="0001" level="any"/>
      </xsl:attribute>
      <xsl:apply-templates select="@*[local-name() != 'id']|node()"/>
    </xsl:copy>
  </xsl:template>

</xsl:stylesheet>
