<?xml version="1.0" encoding="UTF-8"?>
<xsl:transform xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:exslt="http://exslt.org/common" extension-element-prefixes="exslt" version="1.0">
  <xsl:variable name="extension-namespaces">
    <str:node xmlns:str="http://exslt.org/strings"/>
  </xsl:variable>
  <xsl:template match="/">
    <xsl:element name="xsl:transform">
      <xsl:attribute name="version">1.0</xsl:attribute>
      <xsl:copy-of select="exslt:node-set($extension-namespaces)/*/namespace::*"/>
      <xsl:attribute name="extension-element-prefixes">str</xsl:attribute>
      <xsl:element name="xsl:param">
        <xsl:attribute name="name">overwrite-display-text</xsl:attribute>
        <xsl:attribute name="select">true()</xsl:attribute>
      </xsl:element>
      <xsl:apply-templates select="disptext"/>
      <xsl:element name="xsl:template">
        <xsl:attribute name="match">assert[text()]</xsl:attribute>
        <xsl:attribute name="mode">text</xsl:attribute>
        <xsl:element name="xsl:apply-templates"/>
      </xsl:element>
      <xsl:element name="xsl:template">
        <xsl:attribute name="match">evaluate</xsl:attribute>
        <xsl:attribute name="mode">text</xsl:attribute>
        <xsl:element name="xsl:variable">
          <xsl:attribute name="name">op</xsl:attribute>
          <xsl:attribute name="select">@andOr|@operator</xsl:attribute>
        </xsl:element>
        <xsl:element name="xsl:for-each">
          <xsl:attribute name="select">assert|evaluate</xsl:attribute>
          <xsl:element name="xsl:if">
            <xsl:attribute name="test">self::evaluate and (@andOr|@operator) != $op</xsl:attribute>
            <xsl:element name="xsl:text">
              <xsl:value-of select="disptext/operators/openGroup"/>
            </xsl:element>
          </xsl:element>
          <xsl:element name="xsl:apply-templates">
            <xsl:attribute name="select">.</xsl:attribute>
            <xsl:attribute name="mode">text</xsl:attribute>
          </xsl:element>
          <xsl:element name="xsl:if">
            <xsl:attribute name="test">self::evaluate and (@andOr|@operator) != $op</xsl:attribute>
            <xsl:element name="xsl:text">
              <xsl:value-of select="disptext/operators/closeGroup"/>
            </xsl:element>
          </xsl:element>
          <xsl:element name="xsl:if">
            <xsl:attribute name="test">position() != last()</xsl:attribute>
            <xsl:element name="xsl:choose">
              <xsl:element name="xsl:when">
                <xsl:attribute name="test">$op = 'and'</xsl:attribute>
                <xsl:value-of select="disptext/operators/and"/>
              </xsl:element>
              <xsl:element name="xsl:when">
                <xsl:attribute name="test">$op = 'or'</xsl:attribute>
                <xsl:value-of select="disptext/operators/or"/>
              </xsl:element>
            </xsl:element>
          </xsl:element>
        </xsl:element>
      </xsl:element>
      <xsl:element name="xsl:template">
        <xsl:attribute name="match">@applicPropertyValues|@actvalues</xsl:attribute>
        <xsl:attribute name="mode">text</xsl:attribute>
        <xsl:element name="xsl:value-of">
          <xsl:attribute name="select">translate(str:replace(., '|', ', '), '~', '-')</xsl:attribute>
        </xsl:element>
      </xsl:element>
      <xsl:element name="xsl:template">
        <xsl:attribute name="match">applic</xsl:attribute>
        <xsl:element name="xsl:variable">
          <xsl:attribute name="name">disp-name</xsl:attribute>
          <xsl:element name="xsl:choose">
            <xsl:element name="xsl:when">
              <xsl:attribute name="test">parent::status|parent::inlineapplics</xsl:attribute>
              <xsl:text>displaytext</xsl:text>
            </xsl:element>
            <xsl:element name="xsl:otherwise">
              <xsl:text>displayText</xsl:text>
            </xsl:element>
          </xsl:element>
        </xsl:element>
        <xsl:element name="xsl:variable">
          <xsl:attribute name="name">para-name</xsl:attribute>
          <xsl:element name="xsl:choose">
            <xsl:element name="xsl:when">
              <xsl:attribute name="test">parent::status or parent::inlineapplics</xsl:attribute>
              <xsl:text>p</xsl:text>
            </xsl:element>
            <xsl:element name="xsl:otherwise">
              <xsl:text>simplePara</xsl:text>
            </xsl:element>
          </xsl:element>
        </xsl:element>
        <xsl:element name="xsl:variable">
          <xsl:attribute name="name">disp-elem</xsl:attribute>
          <xsl:attribute name="select">displayText|displaytext</xsl:attribute>
        </xsl:element>
        <xsl:element name="xsl:copy">
          <xsl:element name="xsl:apply-templates">
            <xsl:attribute name="select">@*</xsl:attribute>
          </xsl:element>
          <xsl:element name="xsl:choose">
            <xsl:element name="xsl:when">
              <xsl:attribute name="test">$disp-elem and not ($overwrite-display-text)</xsl:attribute>
              <xsl:element name="xsl:apply-templates">
                <xsl:attribute name="select">$disp-elem</xsl:attribute>
              </xsl:element>
            </xsl:element>
            <xsl:element name="xsl:otherwise">
              <xsl:element name="xsl:element">
                <xsl:attribute name="name">{$disp-name}</xsl:attribute>
                <xsl:element name="xsl:element">
                  <xsl:attribute name="name">{$para-name}</xsl:attribute>
                  <xsl:element name="xsl:choose">
                    <xsl:element name="xsl:when">
                      <xsl:attribute name="test">assert|evaluate|expression</xsl:attribute>
                      <xsl:element name="xsl:apply-templates">
                        <xsl:attribute name="select">assert|evaluate|expression</xsl:attribute>
                        <xsl:attribute name="mode">text</xsl:attribute>
                      </xsl:element>
                    </xsl:element>
                    <xsl:element name="xsl:otherwise">
                      <xsl:text>All</xsl:text>
                    </xsl:element>
                  </xsl:element>
                </xsl:element>
              </xsl:element>
            </xsl:element>
          </xsl:element>
          <xsl:element name="xsl:apply-templates">
            <xsl:attribute name="select">assert|evaluate|expression</xsl:attribute>
          </xsl:element>
        </xsl:element>
      </xsl:element>
      <xsl:element name="xsl:template">
        <xsl:attribute name="name">applicPropertyName</xsl:attribute>
        <xsl:element name="xsl:param">
          <xsl:attribute name="name">id</xsl:attribute>
          <xsl:attribute name="select">@applicPropertyIdent|@actidref</xsl:attribute>
        </xsl:element>
        <xsl:element name="xsl:param">
          <xsl:attribute name="name">type</xsl:attribute>
          <xsl:attribute name="select">@applicPropertyType|@actreftype</xsl:attribute>
        </xsl:element>
        <xsl:element name="xsl:variable">
          <xsl:attribute name="name">prop</xsl:attribute>
          <xsl:attribute name="select">//productAttribute[$type='prodattr' and @id=$id]|//prodattr[$type='prodattr' and @id=$id]|//cond[$type='condition' and @id=$id]|//condition[$type='condition' and @id=$id]</xsl:attribute>
        </xsl:element>
        <xsl:element name="xsl:variable">
          <xsl:attribute name="name">disp</xsl:attribute>
          <xsl:attribute name="select">$prop/displayName|$prop/displayname</xsl:attribute>
        </xsl:element>
        <xsl:element name="xsl:variable">
          <xsl:attribute name="name">name</xsl:attribute>
          <xsl:attribute name="select">$prop/name</xsl:attribute>
        </xsl:element>
        <xsl:element name="xsl:choose">
          <xsl:element name="xsl:when">
            <xsl:attribute name="test">$disp</xsl:attribute>
            <xsl:element name="xsl:value-of">
              <xsl:attribute name="select">$disp</xsl:attribute>
            </xsl:element>
          </xsl:element>
          <xsl:element name="xsl:when">
            <xsl:attribute name="test">$name</xsl:attribute>
            <xsl:element name="xsl:value-of">
              <xsl:attribute name="select">$name</xsl:attribute>
            </xsl:element>
          </xsl:element>
          <xsl:element name="xsl:otherwise">
            <xsl:element name="xsl:value-of">
              <xsl:attribute name="select">$id</xsl:attribute>
            </xsl:element>
          </xsl:element>
        </xsl:element>
      </xsl:element>
      <xsl:element name="xsl:template">
        <xsl:attribute name="name">applicPropertyVal</xsl:attribute>
        <xsl:element name="xsl:param">
          <xsl:attribute name="name">id</xsl:attribute>
          <xsl:attribute name="select">@applicPropertyIdent|@actidref</xsl:attribute>
        </xsl:element>
        <xsl:element name="xsl:param">
          <xsl:attribute name="name">type</xsl:attribute>
          <xsl:attribute name="select">@applicPropertyType|@actreftype</xsl:attribute>
        </xsl:element>
        <xsl:element name="xsl:variable">
          <xsl:attribute name="name">prop</xsl:attribute>
          <xsl:attribute name="select">//productAttribute[$type='prodattr' and @id=$id]|//prodattr[$type='prodattr' and @id=$id]|//condType[$type='condition' and @id=//cond[@id=$id]/@condTypeRefId]|//condition[$type='condition' and @id=//cond[@id=$id]/@condtyperef]</xsl:attribute>
        </xsl:element>
        <xsl:element name="xsl:variable">
          <xsl:attribute name="name">values</xsl:attribute>
          <xsl:attribute name="select">@applicPropertyValues|@actvalues</xsl:attribute>
        </xsl:element>
        <xsl:element name="xsl:variable">
          <xsl:attribute name="name">label</xsl:attribute>
          <xsl:attribute name="select">$prop/enumeration[@applicPropertyValues=$values]/@enumerationLabel</xsl:attribute>
        </xsl:element>
        <xsl:element name="xsl:choose">
          <xsl:element name="xsl:when">
            <xsl:attribute name="test">$label</xsl:attribute>
            <xsl:element name="xsl:value-of">
              <xsl:attribute name="select">$label</xsl:attribute>
            </xsl:element>
          </xsl:element>
          <xsl:element name="xsl:otherwise">
            <xsl:element name="xsl:apply-templates">
              <xsl:attribute name="select">$values</xsl:attribute>
              <xsl:attribute name="mode">text</xsl:attribute>
            </xsl:element>
          </xsl:element>
        </xsl:element>
      </xsl:element>
    </xsl:element>
  </xsl:template>
  <xsl:template match="disptext">
    <xsl:element name="xsl:template">
      <xsl:attribute name="match">assert</xsl:attribute>
      <xsl:attribute name="mode">text</xsl:attribute>
      <xsl:element name="xsl:variable">
        <xsl:attribute name="name">ident</xsl:attribute>
        <xsl:attribute name="select">@applicPropertyIdent|@actidref</xsl:attribute>
      </xsl:element>
      <xsl:element name="xsl:variable">
        <xsl:attribute name="name">type</xsl:attribute>
        <xsl:attribute name="select">@applicPropertyType|@actreftype</xsl:attribute>
      </xsl:element>
      <xsl:element name="xsl:choose">
        <xsl:apply-templates select="property"/>
        <xsl:apply-templates select="conditionType"/>
        <xsl:apply-templates select="productAttributes|conditions"/>
        <xsl:apply-templates select="default"/>
      </xsl:element>
    </xsl:element>
  </xsl:template>
  <xsl:template match="property">
    <xsl:element name="xsl:when">
      <xsl:attribute name="test">
        <xsl:text>$ident='</xsl:text>
        <xsl:value-of select="@ident"/>
        <xsl:text>' and $type='</xsl:text>
        <xsl:value-of select="@type"/>
        <xsl:text>'</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="name|text|values"/>
    </xsl:element>
  </xsl:template>
  <xsl:template match="conditionType">
    <xsl:element name="xsl:when">
      <xsl:attribute name="test">
        <xsl:text>$type='condition' and (//cond[@id=$ident]/@condTypeRefId|//condition[@id=$ident]/@condtyperef)='</xsl:text>
        <xsl:value-of select="@ident"/>
        <xsl:text>'</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates select="name|text|values"/>
    </xsl:element>
  </xsl:template>
  <xsl:template match="default">
    <xsl:element name="xsl:when">
      <xsl:attribute name="test">true()</xsl:attribute>
      <xsl:apply-templates select="name|text|values"/>
    </xsl:element>
  </xsl:template>
  <xsl:template match="productAttributes">
    <xsl:element name="xsl:when">
      <xsl:attribute name="test">$type='prodattr'</xsl:attribute>
      <xsl:apply-templates select="name|text|values"/>
    </xsl:element>
  </xsl:template>
  <xsl:template match="conditions">
    <xsl:element name="xsl:when">
      <xsl:attribute name="test">$type='condition'</xsl:attribute>
      <xsl:apply-templates select="name|text|values"/>
    </xsl:element>
  </xsl:template>
  <xsl:template match="name">
    <xsl:element name="xsl:call-template">
      <xsl:attribute name="name">applicPropertyName</xsl:attribute>
    </xsl:element>
  </xsl:template>
  <xsl:template match="text">
    <xsl:element name="xsl:text">
      <xsl:apply-templates/>
    </xsl:element>
  </xsl:template>
  <xsl:template match="values">
    <xsl:choose>
      <xsl:when test="value">
        <xsl:element name="xsl:choose">
          <xsl:apply-templates select="value"/>
          <xsl:element name="xsl:otherwise">
            <xsl:element name="xsl:call-template">
              <xsl:attribute name="name">applicPropertyVal</xsl:attribute>
            </xsl:element>
          </xsl:element>
        </xsl:element>
      </xsl:when>
      <xsl:otherwise>
        <xsl:element name="xsl:call-template">
          <xsl:attribute name="name">applicPropertyVal</xsl:attribute>
        </xsl:element>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template match="value">
    <xsl:element name="xsl:when">
      <xsl:attribute name="test">
        <xsl:text>@applicPropertyValues='</xsl:text>
        <xsl:value-of select="@match"/>
        <xsl:text>' or @actvalues='</xsl:text>
        <xsl:value-of select="@match"/>
        <xsl:text>'</xsl:text>
      </xsl:attribute>
      <xsl:apply-templates/>
    </xsl:element>
  </xsl:template>
</xsl:transform>
