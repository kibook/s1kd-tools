<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

  <xsl:param name="format">SI</xsl:param>

  <xsl:template match="uomDisplay">
    <xsl:element name="xsl:stylesheet">
      <xsl:attribute name="version">1.0</xsl:attribute>
      <xsl:apply-templates select="format[@name = $format]"/>
      <xsl:apply-templates select="uoms"/>
    </xsl:element>
  </xsl:template>

  <xsl:template match="format">
    <xsl:element name="xsl:decimal-format">
      <xsl:attribute name="name">decimal-format</xsl:attribute>
      <xsl:attribute name="decimal-separator">
        <xsl:value-of select="@decimalSeparator"/>
      </xsl:attribute>
      <xsl:attribute name="grouping-separator">
        <xsl:value-of select="@groupingSeparator"/>
      </xsl:attribute>
    </xsl:element>
    <xsl:element name="xsl:variable">
      <xsl:attribute name="name">decimal-separator</xsl:attribute>
      <xsl:element name="xsl:text">
        <xsl:value-of select="@decimalSeparator"/>
      </xsl:element>
    </xsl:element>
    <xsl:element name="xsl:variable">
      <xsl:attribute name="name">grouping-separator</xsl:attribute>
      <xsl:element name="xsl:text">
        <xsl:value-of select="@groupingSeparator"/>
      </xsl:element>
    </xsl:element>
    <xsl:element name="xsl:template">
      <xsl:attribute name="match">@*|node()</xsl:attribute>
      <xsl:element name="xsl:copy">
        <xsl:element name="xsl:apply-templates">
          <xsl:attribute name="select">@*|node()</xsl:attribute>
        </xsl:element>
      </xsl:element>
    </xsl:element>
    <xsl:element name="xsl:template">
      <xsl:attribute name="name">repeat-string</xsl:attribute>
      <xsl:element name="xsl:param">
        <xsl:attribute name="name">string</xsl:attribute>
      </xsl:element>
      <xsl:element name="xsl:param">
        <xsl:attribute name="name">count</xsl:attribute>
      </xsl:element>
      <xsl:element name="xsl:param">
        <xsl:attribute name="name">group</xsl:attribute>
      </xsl:element>
      <xsl:element name="xsl:param">
        <xsl:attribute name="name">separator</xsl:attribute>
      </xsl:element>
      <xsl:element name="xsl:if">
        <xsl:attribute name="test">$count &gt; 0</xsl:attribute>
        <xsl:element name="xsl:value-of">
          <xsl:attribute name="select">$string</xsl:attribute>
        </xsl:element>
        <xsl:element name="xsl:if">
          <xsl:attribute name="test">$group and $count &gt; 1 and $count mod $group = 1</xsl:attribute>
          <xsl:element name="xsl:value-of">
            <xsl:attribute name="select">$separator</xsl:attribute>
          </xsl:element>
        </xsl:element>
        <xsl:element name="xsl:call-template">
          <xsl:attribute name="name">repeat-string</xsl:attribute>
          <xsl:element name="xsl:with-param">
            <xsl:attribute name="name">string</xsl:attribute>
            <xsl:attribute name="select">$string</xsl:attribute>
          </xsl:element>
          <xsl:element name="xsl:with-param">
            <xsl:attribute name="name">count</xsl:attribute>
            <xsl:attribute name="select">$count - 1</xsl:attribute>
          </xsl:element>
          <xsl:element name="xsl:with-param">
            <xsl:attribute name="name">group</xsl:attribute>
            <xsl:attribute name="select">$group</xsl:attribute>
          </xsl:element>
          <xsl:element name="xsl:with-param">
            <xsl:attribute name="name">separator</xsl:attribute>
            <xsl:attribute name="select">$separator</xsl:attribute>
          </xsl:element>
        </xsl:element>
      </xsl:element>
    </xsl:element>
    <xsl:element name="xsl:template">
      <xsl:attribute name="name">generate-number-format</xsl:attribute>
      <xsl:element name="xsl:param">
        <xsl:attribute name="name">value</xsl:attribute>
        <xsl:attribute name="select">.</xsl:attribute>
      </xsl:element>
      <xsl:element name="xsl:variable">
        <xsl:attribute name="name">abs</xsl:attribute>
        <xsl:element name="xsl:choose">
          <xsl:element name="xsl:when">
            <xsl:attribute name="test">starts-with($value, '-')</xsl:attribute>
            <xsl:element name="xsl:value-of">
              <xsl:attribute name="select">substring-after($value, '-')</xsl:attribute>
            </xsl:element>
          </xsl:element>
          <xsl:element name="xsl:otherwise">
            <xsl:element name="xsl:value-of">
              <xsl:attribute name="select">$value</xsl:attribute>
            </xsl:element>
          </xsl:element>
        </xsl:element>
      </xsl:element>
      <xsl:element name="xsl:variable">
        <xsl:attribute name="name">has-decimal</xsl:attribute>
        <xsl:attribute name="select">contains($abs, '.')</xsl:attribute>
      </xsl:element>
      <xsl:element name="xsl:call-template">
        <xsl:attribute name="name">repeat-string</xsl:attribute>
        <xsl:element name="xsl:with-param">
          <xsl:attribute name="name">string</xsl:attribute>
          <xsl:text>0</xsl:text>
        </xsl:element>
        <xsl:element name="xsl:with-param">
          <xsl:attribute name="name">count</xsl:attribute>
          <xsl:element name="xsl:choose">
            <xsl:element name="xsl:when">
              <xsl:attribute name="test">$has-decimal</xsl:attribute>
              <xsl:element name="xsl:value-of">
                <xsl:attribute name="select">string-length(substring-before($abs, '.'))</xsl:attribute>
              </xsl:element>
            </xsl:element>
            <xsl:element name="xsl:otherwise">
              <xsl:element name="xsl:value-of">
                <xsl:attribute name="select">string-length($abs)</xsl:attribute>
              </xsl:element>
            </xsl:element>
          </xsl:element>
        </xsl:element>
        <xsl:element name="xsl:with-param">
          <xsl:attribute name="name">group</xsl:attribute>
          <xsl:text>3</xsl:text>
        </xsl:element>
        <xsl:element name="xsl:with-param">
          <xsl:attribute name="name">separator</xsl:attribute>
          <xsl:element name="xsl:value-of">
            <xsl:attribute name="select">$grouping-separator</xsl:attribute>
          </xsl:element>
        </xsl:element>
      </xsl:element>
      <xsl:element name="xsl:if">
        <xsl:attribute name="test">$has-decimal</xsl:attribute>
        <xsl:element name="xsl:value-of">
          <xsl:attribute name="select">$decimal-separator</xsl:attribute>
        </xsl:element>
        <xsl:element name="xsl:call-template">
          <xsl:attribute name="name">repeat-string</xsl:attribute>
          <xsl:element name="xsl:with-param">
            <xsl:attribute name="name">string</xsl:attribute>
            <xsl:text>0</xsl:text>
          </xsl:element>
          <xsl:element name="xsl:with-param">
            <xsl:attribute name="name">count</xsl:attribute>
            <xsl:attribute name="select">string-length(substring-after($value, '.'))</xsl:attribute>
          </xsl:element>
        </xsl:element>
      </xsl:element>
    </xsl:element>
    <xsl:element name="xsl:template">
      <xsl:attribute name="name">format-quantity-value</xsl:attribute>
      <xsl:element name="xsl:param">
        <xsl:attribute name="name">value</xsl:attribute>
        <xsl:attribute name="select">.</xsl:attribute>
      </xsl:element>
      <xsl:element name="xsl:variable">
        <xsl:attribute name="name">format</xsl:attribute>
        <xsl:element name="xsl:call-template">
          <xsl:attribute name="name">generate-number-format</xsl:attribute>
          <xsl:element name="xsl:with-param">
            <xsl:attribute name="name">value</xsl:attribute>
            <xsl:attribute name="select">$value</xsl:attribute>
          </xsl:element>
        </xsl:element>
      </xsl:element>
      <xsl:element name="xsl:value-of">
        <xsl:attribute name="select">format-number($value, $format, 'decimal-format')</xsl:attribute>
      </xsl:element>
    </xsl:element>
    <xsl:element name="xsl:template">
      <xsl:attribute name="match">quantity</xsl:attribute>
      <xsl:choose>
        <xsl:when test="../wrapInto/*">
          <xsl:apply-templates select="../wrapInto/*"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="quantity"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:element>
    <xsl:element name="xsl:template">
      <xsl:attribute name="match">quantityGroup</xsl:attribute>
      <xsl:element name="xsl:choose">
        <xsl:element name="xsl:when">
          <xsl:attribute name="test">@quantityGroupType = 'minimum'</xsl:attribute>
          <xsl:element name="xsl:choose">
            <xsl:element name="xsl:when">
              <xsl:attribute name="test">following-sibling::quantityGroup</xsl:attribute>
              <xsl:text>from </xsl:text>
            </xsl:element>
            <xsl:element name="xsl:otherwise">
              <xsl:text>at least </xsl:text>
            </xsl:element>
          </xsl:element>
        </xsl:element>
        <xsl:element name="xsl:when">
          <xsl:attribute name="test">@quantityGroupType = 'maximum'</xsl:attribute>
          <xsl:element name="xsl:choose">
            <xsl:element name="xsl:when">
              <xsl:attribute name="test">preceding-sibling::quantityGroup</xsl:attribute>
              <xsl:text> to </xsl:text>
            </xsl:element>
            <xsl:element name="xsl:otherwise">
              <xsl:text>up to </xsl:text>
            </xsl:element>
          </xsl:element>
        </xsl:element>
      </xsl:element>
      <xsl:element name="xsl:for-each">
        <xsl:attribute name="select">quantityValue|quantityTolerance</xsl:attribute>
        <xsl:element name="xsl:if">
          <xsl:attribute name="test">position() != 1</xsl:attribute>
          <xsl:element name="xsl:text">
            <xsl:text> </xsl:text>
          </xsl:element>
        </xsl:element>
        <xsl:element name="xsl:apply-templates">
          <xsl:attribute name="select">.</xsl:attribute>
        </xsl:element>
      </xsl:element>
      <xsl:element name="xsl:apply-templates">
        <xsl:attribute name="select">@quantityUnitOfMeasure</xsl:attribute>
      </xsl:element>
    </xsl:element>
    <xsl:element name="xsl:template">
      <xsl:attribute name="match">quantityValue</xsl:attribute>
      <xsl:element name="xsl:call-template">
        <xsl:attribute name="name">format-quantity-value</xsl:attribute>
      </xsl:element>
      <xsl:element name="xsl:apply-templates">
        <xsl:attribute name="select">@quantityUnitOfMeasure</xsl:attribute>
      </xsl:element>
    </xsl:element>
    <xsl:element name="xsl:template">
      <xsl:attribute name="match">quantityTolerance</xsl:attribute>
      <xsl:element name="xsl:call-template">
        <xsl:attribute name="name">tolerance-type</xsl:attribute>
      </xsl:element>
      <xsl:element name="xsl:call-template">
        <xsl:attribute name="name">format-quantity-value</xsl:attribute>
      </xsl:element>
      <xsl:element name="xsl:apply-templates">
        <xsl:attribute name="select">@quantityUnitOfMeasure</xsl:attribute>
      </xsl:element>
    </xsl:element>
    <xsl:element name="xsl:template">
      <xsl:attribute name="name">tolerance-type</xsl:attribute>
      <xsl:element name="xsl:param">
        <xsl:attribute name="name">type</xsl:attribute>
        <xsl:attribute name="select">@quantityToleranceType</xsl:attribute>
      </xsl:element>
      <xsl:element name="xsl:choose">
        <xsl:element name="xsl:when">
          <xsl:attribute name="test">$type = 'plus'</xsl:attribute>
          <xsl:text>+</xsl:text>
        </xsl:element>
        <xsl:element name="xsl:when">
          <xsl:attribute name="test">$type = 'minus'</xsl:attribute>
          <xsl:text>-</xsl:text>
        </xsl:element>
        <xsl:element name="xsl:otherwise">
          <xsl:text>± </xsl:text>
        </xsl:element>
      </xsl:element>
    </xsl:element>
  </xsl:template>

  <xsl:template name="quantity">
    <xsl:element name="xsl:apply-templates"/>
    <xsl:element name="xsl:if">
      <xsl:attribute name="test">@quantityTypeSpecifics</xsl:attribute>
      <xsl:element name="xsl:text">
        <xsl:text> </xsl:text>
      </xsl:element>
      <xsl:element name="xsl:value-of">
        <xsl:attribute name="select">@quantityTypeSpecifics</xsl:attribute>
      </xsl:element>
    </xsl:element>
  </xsl:template>

  <xsl:template match="wrapInto/*">
    <xsl:copy>
      <xsl:copy-of select="@*"/>
      <xsl:call-template name="quantity"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="uoms">
    <xsl:element name="xsl:template">
      <xsl:attribute name="match">@quantityUnitOfMeasure</xsl:attribute>
      <xsl:element name="xsl:choose">
        <xsl:apply-templates select="uom"/>
        <xsl:element name="xsl:otherwise">
          <xsl:element name="xsl:text">
            <xsl:text> </xsl:text>
          </xsl:element>
          <xsl:element name="xsl:value-of">
            <xsl:attribute name="select">.</xsl:attribute>
          </xsl:element>
        </xsl:element>
      </xsl:element>
    </xsl:element>
  </xsl:template>

  <xsl:template match="uom">
    <xsl:element name="xsl:when">
      <xsl:attribute name="test">
        <xsl:text>. = '</xsl:text>
        <xsl:value-of select="@name"/>
        <xsl:text>'</xsl:text>
      </xsl:attribute>
      <xsl:copy-of select="node()"/>
    </xsl:element>
  </xsl:template>

</xsl:stylesheet>
