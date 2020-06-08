<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
  xmlns:xlink="http://www.w3.org/1999/xlink"
  xmlns:dm="http://www.s1000d.org/dm"
  xmlns:pm="http://www.s1000d.org/pm"
  version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="rdf:Description"/>

  <xsl:template match="@xlink:*"/>

  <xsl:template match="dm:*|pm:*">
    <xsl:element name="{local-name()}">
      <xsl:apply-templates select="@*|node()"/>
    </xsl:element>
  </xsl:template>

  <xsl:template match="@dm:*|@pm:*">
    <xsl:attribute name="{local-name()}">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

</xsl:stylesheet>
