<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="ein">
    <xsl:variable name="einnbr" select="@einnbr"/>
    <xsl:variable name="eintype" select="@eintype"/>
    <xsl:variable name="mfc" select="@mfc"/>
    <xsl:variable name="einid" select="
      //einid[
        $einnbr = @einnbr and
        (not($eintype) or $eintype = @eintype) and
        (not($mfc) or $mfc = @mfc)
      ]"/>
    <xsl:variable name="eininfo" select="$einid/parent::eininfo"/>
    <xsl:variable name="einalt" select="$eininfo/einalt[1]"/>
    <ein>
      <xsl:choose>
        <xsl:when test="$eininfo">
          <xsl:apply-templates select="@id"/>
          <xsl:apply-templates select="$einid/@einnbr"/>
          <xsl:apply-templates select="$einid/@eintype"/>
          <xsl:apply-templates select="$einid/@mfc"/>
          <xsl:choose>
            <xsl:when test="$einalt/nomen">
              <xsl:apply-templates select="$einalt/nomen"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:apply-templates select="$eininfo/nomen"/>
            </xsl:otherwise>
          </xsl:choose>
          <xsl:apply-templates select="$eininfo/refs"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="@*|node()"/>
        </xsl:otherwise>
      </xsl:choose>
    </ein>
  </xsl:template>

</xsl:stylesheet>
