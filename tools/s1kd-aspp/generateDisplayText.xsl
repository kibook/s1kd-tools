<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0"
  xmlns:str="http://exslt.org/strings"
  extension-element-prefixes="str">

  <xsl:param name="overwrite-display-text" select="true()"/>

  <xsl:template match="assert" mode="text">
    <xsl:call-template name="applicPropertyName"/>
    <xsl:text>: </xsl:text>
    <xsl:call-template name="applicPropertyVal"/>
  </xsl:template>

  <xsl:template match="assert[text()]" mode="text">
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="evaluate" mode="text">
    <xsl:variable name="op" select="@andOr|@operator"/>
    <xsl:for-each select="assert|evaluate">
      <xsl:if test="self::evaluate and (@andOr|@operator) != $op">
        <xsl:text>(</xsl:text>
      </xsl:if>
      <xsl:apply-templates select="." mode="text"/>
      <xsl:if test="self::evaluate and (@andOr|@operator) != $op">
        <xsl:text>)</xsl:text>
      </xsl:if>
      <xsl:if test="position() != last()">
        <xsl:text> </xsl:text>
        <xsl:value-of select="$op"/>
        <xsl:text> </xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="@applicPropertyValues|@actvalues" mode="text">
    <xsl:value-of select="translate(str:replace(., '|', ', '), '~', '-')"/>
  </xsl:template>

  <xsl:template match="applic">
    <xsl:variable name="disp-name">
      <xsl:choose>
        <xsl:when test="parent::status|parent::inlineapplics">displaytext</xsl:when>
        <xsl:otherwise>displayText</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="para-name">
      <xsl:choose>
        <xsl:when test="parent::status|parent::inlineapplics">p</xsl:when>
        <xsl:otherwise>simplePara</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="disp-elem" select="displayText|displaytext"/>
    <applic>
      <xsl:apply-templates select="@*"/>
      <xsl:choose>
        <xsl:when test="$disp-elem and not($overwrite-display-text)">
          <xsl:apply-templates select="$disp-elem"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:element name="{$disp-name}">
            <xsl:element name="{$para-name}">
              <xsl:if test="not(assert|evaluate|expression)">
                <xsl:text>All</xsl:text>
              </xsl:if>
              <xsl:apply-templates select="assert|evaluate|expression" mode="text"/>
            </xsl:element>
          </xsl:element>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:apply-templates select="assert|evaluate|expression"/>
    </applic>
  </xsl:template>

  <xsl:template name="applicPropertyName">
    <xsl:param name="id" select="@applicPropertyIdent|@actidref"/>
    <xsl:param name="type" select="@applicPropertyType|@actreftype"/>
    <xsl:variable name="prop" select="//productAttribute[$type='prodattr' and @id=$id]|//prodattr[$type='prodattr' and @id=$id]|//cond[$type='condition' and @id=$id]|//condition[$type='condition' and @id=$id]"/>
    <xsl:variable name="disp" select="$prop/displayName|$prop/displayname"/>
    <xsl:variable name="name" select="$prop/name"/>
    <xsl:choose>
      <xsl:when test="$disp">
        <xsl:value-of select="$disp"/>
      </xsl:when>
      <xsl:when test="$name">
        <xsl:value-of select="$name"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$id"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="applicPropertyVal">
    <xsl:param name="id" select="@applicPropertyIdent|@actidref"/>
    <xsl:param name="type" select="@applicPropertyType|@actreftype"/>
    <xsl:variable name="prop" select="//productAttribute[$type='prodattr' and @id=$id]|//prodattr[$type='prodattr' and @id=$id]|//condType[$type='condition' and @id=//cond[@id=$id]/@condTypeRefId]|//condition[$type='condition' and @id=//cond[@id=$id]/@condtyperef]"/>
    <xsl:variable name="values" select="@applicPropertyValues|@actvalues"/>
    <xsl:variable name="label" select="$prop/enumeration[@applicPropertyValues=$values]/@enumerationLabel"/>
    <xsl:choose>
      <xsl:when test="$label">
        <xsl:value-of select="$label"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates select="$values" mode="text"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
