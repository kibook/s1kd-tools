<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0"
  xmlns:str="http://exslt.org/strings"
  extension-element-prefixes="str">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="applic">
    <applic>
      <xsl:apply-templates select="@*"/>
      <displayText>
        <simplePara>
          <xsl:if test="not(assert|evaluate|expression)">
            <xsl:text>All</xsl:text>
          </xsl:if>
          <xsl:apply-templates select="assert|evaluate|expression" mode="text"/>
        </simplePara>
      </displayText>
      <xsl:apply-templates select="assert|evaluate|expression"/>
    </applic>
  </xsl:template>

  <xsl:template match="assert" mode="text">
    <xsl:call-template name="applicPropertyName"/>
    <xsl:text>: </xsl:text>
    <xsl:apply-templates select="@applicPropertyValues" mode="text"/>
  </xsl:template>

  <xsl:template name="applicPropertyName">
    <xsl:param name="id" select="@applicPropertyIdent"/>
    <xsl:param name="type" select="@applicPropertyType"/>
    <xsl:variable name="productAttribute" select="//productAttribute[$type='prodattr' and @id=$id]"/>
    <xsl:variable name="cond" select="//cond[$type='condition' and @id=$id]"/>
    <xsl:choose>
      <xsl:when test="$productAttribute/displayName">
        <xsl:value-of select="$productAttribute/displayName"/>
      </xsl:when>
      <xsl:when test="$productAttribute/name">
        <xsl:value-of select="$productAttribute/name"/>
      </xsl:when>
      <xsl:when test="$cond/displayName">
        <xsl:value-of select="$cond/displayName"/>
      </xsl:when>
      <xsl:when test="$cond/name">
        <xsl:value-of select="$cond/name"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$id"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="@applicPropertyValues" mode="text">
    <xsl:value-of select="translate(str:replace(., '|', ', '), '~', '-')"/>
  </xsl:template>

  <xsl:template match="assert[not(@*)]" mode="text">
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="evaluate" mode="text">
    <xsl:variable name="op" select="@andOr"/>
    <xsl:for-each select="assert|evaluate">
      <xsl:if test="self::evaluate and @andOr != $op">
        <xsl:text>(</xsl:text>
      </xsl:if>
      <xsl:apply-templates select="." mode="text"/>
      <xsl:if test="self::evaluate and @andOr != $op">
        <xsl:text>)</xsl:text>
      </xsl:if>
      <xsl:if test="position() != last()">
        <xsl:text> </xsl:text>
        <xsl:value-of select="$op"/>
        <xsl:text> </xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

</xsl:stylesheet>
