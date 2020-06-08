<?xml version="1.0" encoding="UTF-8"?>
<xsl:transform
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xi="http://www.w3.org/2001/XInclude"
  version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="pmEntry[not(.//dmRef|.//pmRef|.//externalPubRef|.//xi:include)]"/>
  <xsl:template match="pmentry[not(.//refdm|.//refpm|.//refextp|.//xi:include)]"/>

</xsl:transform>
