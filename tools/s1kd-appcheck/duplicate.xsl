<?xml version="1.0" encoding="UTF-8"?>
<xsl:transform
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:s1kd-appcheck="urn:s1kd-tools:s1kd-appcheck"
  xmlns:str="http://exslt.org/strings"
  extension-element-prefixes="str"
  version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="@*|*" mode="copy">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()" mode="copy"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="displayText|disptext" mode="copy">
    <displayText>
      <xsl:apply-templates select="simplePara|p" mode="copy"/>
    </displayText>
  </xsl:template>

  <xsl:template match="simplePara|p" mode="copy">
    <simplePara>
      <xsl:value-of select="."/>
    </simplePara>
  </xsl:template>

  <xsl:template match="assert" mode="copy">
    <xsl:variable name="ident" select="@applicPropertyIdent|@actidref"/>
    <xsl:variable name="type" select="@applicPropertyType|@actreftype"/>
    <xsl:variable name="values" select="str:tokenize(@applicPropertyValues|@actvalues, '|')"/>
    <xsl:choose>
      <xsl:when test="count($values) = 1">
        <assert applicPropertyIdent="{$ident}" applicPropertyType="{$type}" applicPropertyValues="{$values[1]}"/>
      </xsl:when>
      <xsl:otherwise>
        <evaluate andOr="or">
          <xsl:for-each select="$values">
            <xsl:sort select="."/>
            <assert applicPropertyIdent="{$ident}" applicPropertyType="{$type}" applicPropertyValues="{.}"/>
          </xsl:for-each>
        </evaluate>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="evaluate" mode="copy">
    <evaluate andOr="{@andOr|@operator}">
      <xsl:apply-templates select="assert" mode="copy">
        <xsl:sort select="@applicPropertyIdent|@actidref"/>
        <xsl:sort select="@applicPropertyType|@actreftype"/>
        <xsl:sort select="@applicPropertyValues|@actvalues"/>
      </xsl:apply-templates>
      <xsl:apply-templates select="evaluate" mode="copy">
        <xsl:sort select="@andOr|@operator"/>
      </xsl:apply-templates>
    </evaluate>
  </xsl:template>

  <xsl:template match="applic">
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <s1kd-appcheck:annotation>
        <xsl:choose>
          <xsl:when test="assert|evaluate|expression">
            <xsl:apply-templates select="assert|evaluate|expression" mode="copy"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:apply-templates select="displayText|disptext" mode="copy"/>
          </xsl:otherwise>
        </xsl:choose>
      </s1kd-appcheck:annotation>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

</xsl:transform>
