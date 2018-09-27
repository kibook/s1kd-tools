<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dm="http://www.s1000d.org/dm"
  xmlns:pm="http://www.s1000d.org/pm"
  xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
  xmlns:dc="http://www.purl.org/dc/elements/1.1/"
  xmlns:xlink="http://www.w3.org/1999/xlink"
  exclude-result-prefixes="dm pm"
  version="1.0">

  <!-- Apply the S1000D dm/pm namespaces to elements. -->

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="/dmodule">
    <dm:dmodule>
      <xsl:apply-templates select="@*|node()"/>
    </dm:dmodule>
  </xsl:template>

  <xsl:template match="/pm">
    <pm:pm>
      <xsl:apply-templates select="@*|node()"/>
    </pm:pm>
  </xsl:template>

  <xsl:template match="*[ancestor::dmodule and namespace-uri() = '']">
    <xsl:element name="dm:{local-name()}">
      <xsl:apply-templates select="@*|node()"/>
    </xsl:element>
  </xsl:template>

  <xsl:template match="@*[ancestor::dmodule and namespace-uri() = '']">
    <xsl:attribute name="dm:{local-name()}">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="*[ancestor::pm and namespace-uri() = '']">
    <xsl:element name="pm:{local-name()}">
      <xsl:apply-templates select="@*|node()"/>
    </xsl:element>
  </xsl:template>

  <xsl:template match="@*[ancestor::pm and namespace-uri() = '']">
    <xsl:attribute name="pm:{local-name()}">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

</xsl:stylesheet>
