<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:param name="duplicate" select="false()"/>
  <xsl:param name="user-format"/>

  <xsl:variable name="default-format">0.##</xsl:variable>
 
  <xsl:template match="uom">
    <xsl:element name="xsl:stylesheet">
      <xsl:attribute name="version">1.0</xsl:attribute>
      <xsl:element name="xsl:variable">
        <xsl:attribute name="name">pi</xsl:attribute>
        <xsl:attribute name="select">3.14159265359</xsl:attribute>
      </xsl:element>
      <xsl:element name="xsl:template">
        <xsl:attribute name="match">@*|node()</xsl:attribute>
        <xsl:element name="xsl:copy">
          <xsl:element name="xsl:apply-templates">
            <xsl:attribute name="select">@*|node()</xsl:attribute>
          </xsl:element>
        </xsl:element>
      </xsl:element>
      <xsl:if test="convert">
        <xsl:element name="xsl:template">
          <xsl:attribute name="match">
            <xsl:text>quantityValue|qtyvalue|</xsl:text>
            <xsl:text>quantityTolerance|qtytolerance|</xsl:text>
            <xsl:text>quantity[not(*)]</xsl:text>
          </xsl:attribute>
          <xsl:element name="xsl:variable">
            <xsl:attribute name="name">uom</xsl:attribute>
            <xsl:attribute name="select">
              <xsl:text>@quantityUnitOfMeasure|@qtyuom|</xsl:text>
              <xsl:text>parent::quantityGroup/@quantityUnitOfMeasure|parent::qtygrp/@qtyuom|</xsl:text>
              <xsl:text>ancestor-or-self::quantity/@quantityTypeSpecifics</xsl:text>
            </xsl:attribute>
          </xsl:element>
          <xsl:element name="xsl:variable">
            <xsl:attribute name="name">value</xsl:attribute>
            <xsl:attribute name="select">.</xsl:attribute>
          </xsl:element>
          <xsl:element name="xsl:copy">
            <xsl:element name="xsl:apply-templates">
              <xsl:attribute name="select">@*</xsl:attribute>
            </xsl:element>
            <xsl:element name="xsl:choose">
              <xsl:apply-templates select="convert">
                <xsl:with-param name="uom-format" select="@format"/>
              </xsl:apply-templates>
              <xsl:element name="xsl:otherwise">
                <xsl:element name="xsl:value-of">
                  <xsl:attribute name="select">.</xsl:attribute>
                </xsl:element>
              </xsl:element>
            </xsl:element>
          </xsl:element>
        </xsl:element>
        <xsl:element name="xsl:template">
          <xsl:attribute name="match">
            <xsl:text>@quantityUnitOfMeasure|@qtyuom|</xsl:text>
            <xsl:text>@quantityTypeSpecifics</xsl:text>
          </xsl:attribute>
          <xsl:element name="xsl:attribute">
            <xsl:attribute name="name">{name()}</xsl:attribute>
            <xsl:element name="xsl:choose">
              <xsl:apply-templates select="convert" mode="attr"/>
              <xsl:element name="xsl:otherwise">
                <xsl:element name="xsl:value-of">
                  <xsl:attribute name="select">.</xsl:attribute>
                </xsl:element>
              </xsl:element>
            </xsl:element>
          </xsl:element>
        </xsl:element>
      </xsl:if>
    </xsl:element>
  </xsl:template>

  <xsl:template match="convert">
    <xsl:param name="uom-format"/>
    <xsl:variable name="format">
      <xsl:choose>
        <xsl:when test="$user-format">
          <xsl:value-of select="$user-format"/>
        </xsl:when>
        <xsl:when test="$uom-format">
          <xsl:value-of select="$uom-format"/>
        </xsl:when>
        <xsl:when test="@format">
          <xsl:value-of select="@format"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$default-format"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:element name="xsl:when">
      <xsl:attribute name="test">
        <xsl:text>$uom = '</xsl:text>
        <xsl:value-of select="@from"/>
        <xsl:text>'</xsl:text>
      </xsl:attribute>
      <xsl:if test="$duplicate and @from != @to">
        <xsl:element name="xsl:processing-instruction">
          <xsl:attribute name="name">s1kd-uom_CONVERTED</xsl:attribute>
        </xsl:element>
      </xsl:if>
      <xsl:element name="xsl:value-of">
        <xsl:attribute name="select">
          <xsl:choose>
            <xsl:when test="@formula">
              <xsl:text>format-number(</xsl:text>
              <xsl:value-of select="@formula"/>
              <xsl:text>, '</xsl:text>
              <xsl:value-of select="$format"/>
              <xsl:text>')</xsl:text>
            </xsl:when>
            <xsl:otherwise>
              <xsl:text>$value</xsl:text>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
      </xsl:element>
    </xsl:element>
  </xsl:template>

  <xsl:template match="convert" mode="attr">
    <xsl:element name="xsl:when">
      <xsl:attribute name="test">
        <xsl:text>. = '</xsl:text>
        <xsl:value-of select="@from"/>
        <xsl:text>'</xsl:text>
      </xsl:attribute>
      <xsl:value-of select="@to"/>
    </xsl:element>
  </xsl:template>

</xsl:stylesheet>
