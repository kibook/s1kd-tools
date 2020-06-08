<?xml version="1.0" encoding="UTF-8"?>
<xsl:transform
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xi="http://www.w3.org/2001/XInclude"
  version="1.0">

  <xsl:param name="INF_PREFIX">s1kd-flatten: INFO: </xsl:param>

  <xsl:variable name="QUIET" select="0"/>
  <xsl:variable name="NORMAL" select="1"/>
  <xsl:variable name="VERBOSE" select="2"/>
  <xsl:variable name="DEBUG" select="3"/>

  <xsl:param name="verbosity" select="$NORMAL"/>

  <xsl:key name="IDENT" match="*[@IDENT]" use="@IDENT"/>

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="*[@IDENT]">
    <xsl:variable name="IDENT" select="@IDENT"/>
    <xsl:if test="$verbosity &gt;= $DEBUG">
      <xsl:message>
        <xsl:value-of select="$INF_PREFIX"/>
        <xsl:text>Checking reference </xsl:text>
        <xsl:value-of select="$IDENT"/>
        <xsl:text>...</xsl:text>
      </xsl:message>
    </xsl:if>
    <xsl:choose>
      <xsl:when test="generate-id() = generate-id(key('IDENT', $IDENT))">
        <xsl:copy>
          <xsl:apply-templates select="@*[name() != 'IDENT']|node()"/>
        </xsl:copy>
      </xsl:when>
      <xsl:when test="$verbosity &gt;= $VERBOSE">
        <xsl:message>
          <xsl:value-of select="$INF_PREFIX"/>
          <xsl:text>Removed duplicate reference: </xsl:text>
          <xsl:value-of select="$IDENT"/>
        </xsl:message>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

</xsl:transform>
