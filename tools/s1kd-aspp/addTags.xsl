<?xml version="1.0" encoding="UTF-8"?>
<xsl:transform
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

  <xsl:param name="mode" select="'pi'"/>

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="*[@applicRefId]">
    <xsl:variable name="applicRefId" select="@applicRefId"/>
    <xsl:variable name="applic" select="//applic[@id = $applicRefId]"/>
    <xsl:choose>
      <xsl:when test="$mode = 'pi'">
        <xsl:apply-templates select="$applic" mode="pi"/>
      </xsl:when>
      <xsl:when test="$mode = 'comment'">
        <xsl:apply-templates select="$applic" mode="comment"/>
      </xsl:when>
    </xsl:choose>
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="applic" mode="pi">
    <xsl:processing-instruction name="s1kd-aspp">
      <xsl:text>Applicable to: </xsl:text>
      <xsl:apply-templates select="displayText/simplePara"/>
    </xsl:processing-instruction>
  </xsl:template>

  <xsl:template match="applic" mode="comment">
    <xsl:comment>
      <xsl:text>Applicable to: </xsl:text>
      <xsl:apply-templates select="displayText/simplePara"/>
    </xsl:comment>
  </xsl:template>

  <xsl:template match="processing-instruction('s1kd-aspp')"/>

</xsl:transform>
